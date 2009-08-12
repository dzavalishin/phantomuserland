/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no (but GC is not finished)
 *
 *
**/


#include <phantom_assert.h>
#include <phantom_libc.h>

#include <hal.h>
#include <kernel/snap_sync.h>


#include "vm/alloc.h"
#include "vm/internal.h"
#include "vm/root.h"
#include "vm/exec.h"
#include "vm/object_flags.h"


static void gc_roots_to_area();
static hal_spinlock_t dynroots_lock;

static hal_spinlock_t refzero_spinlock;
static void refzero_process_children( pvm_object_storage_t *o );



// Stick to the tradition
#define PVM_END_MEM_MARKER 0xDEADBEEF

// TODO: mark end of memory with some cookie and check for it
// to be there in memcheck and allocator

// BUG: Need global allocator and gc semaphore

static void pvm_init_object_header(struct object_PVM_ALLOC_Header *h, unsigned int size)
{
    h->object_start_marker = PVM_OBJECT_START_MARKER;
    // dont' touch alloc_flags
    h->gc_flags = 0;
    // dont' touch exact_size
    h->refCount = 1;

    h->exact_size = size;
}




// taken if allocation is not possible now - by allocator and critical part of GC
static hal_mutex_t  alloc_mutex;
// allocator waits for GC to give him a chance
static volatile int     alloc_request_gc_pause = 0;

// Allocator and GC work in these bounds. NB! - pvm_object_space_end is OUT of arena
static void * pvm_object_space_start;
static void * pvm_object_space_end;

// Last position where allocator finished looking for objects.
static void * pvm_object_space_alloc_current_position;


struct pvm_object_storage *get_root_object_storage() { return pvm_object_space_start; }




void pvm_alloc_init( void * _pvm_object_space_start, unsigned int size )
{
    hal_spin_init( &refzero_spinlock );
    hal_spin_init( &dynroots_lock );

    pvm_object_space_start = _pvm_object_space_start;
    pvm_object_space_end = pvm_object_space_start + size;

    pvm_object_space_alloc_current_position = pvm_object_space_start;

    if( hal_mutex_init( &alloc_mutex ) )
        panic("Can't init allocator mutex");

}


pvm_object_storage_t *pvm_alloc_wrap_to_next_object(pvm_object_storage_t *op)
{
    assert(op->_ah.object_start_marker == PVM_OBJECT_START_MARKER);
    void *o = (void *)op;
    o += op->_ah.exact_size;
    if( o >= pvm_object_space_end )
    {
        o = pvm_object_space_start;
        printf("alloc wrap??");
    }
    return (struct pvm_object_storage *)o;
}


pvm_object_storage_t *pvm_alloc_following_object(pvm_object_storage_t *op)
{
    assert(op->_ah.object_start_marker == PVM_OBJECT_START_MARKER);
    void *o = (void *)op;
    o += op->_ah.exact_size;
    if( o >= pvm_object_space_end )
        return 0;

    if( *((int *)o) != PVM_OBJECT_START_MARKER)
        return 0;

    return (struct pvm_object_storage *)o;
}


#if 1
static inline int pvm_alloc_is_object(struct pvm_object_storage *o)
{
    return o->_ah.object_start_marker == PVM_OBJECT_START_MARKER;
}
#else
#define pvm_alloc_is_object(o) ((o)->_ah.object_start_marker == PVM_OBJECT_START_MARKER)
#endif


#if 1
static inline int pvm_alloc_is_free_object(struct pvm_object_storage *o)
{
    return
        (o->_ah.object_start_marker == PVM_OBJECT_START_MARKER)
        &&
        (!(o->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED))
        ;
}
#else
#define pvm_alloc_is_free_object(o) (\
    ((o)->_ah.object_start_marker == PVM_OBJECT_START_MARKER) \
    && \
    (!((o)->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED)))
#endif

#define check_out_of_mem() do {                                         \
	if(curr == pvm_object_space_alloc_current_position)          \
        { \
            return 0;\
            pvm_exec_throw( "out of memory" );                          \
    	    }                                                           \
    	} while(0);


void pvm_alloc_clear_mem(void)
{
    assert(pvm_object_space_start != 0);
    *((int *)pvm_object_space_start) = PVM_END_MEM_MARKER;
}


// Find a piece of mem of given or bigger size.
static struct pvm_object_storage *pvm_find(unsigned int size)
{
    //size += 1000; // alloc debug, TEMP, BUG

    struct pvm_object_storage *result = 0;

    struct pvm_object_storage *curr = pvm_object_space_alloc_current_position;

    int wrapped = 0;

    while(result == 0)
    {
        // BUG must be >= ?
        if( wrapped && ((void *)curr > pvm_object_space_alloc_current_position) )
            return 0;

        if( (void *)curr < pvm_object_space_alloc_current_position )
            wrapped = 1;

        if(!pvm_alloc_is_object(curr))
        {
            //assert( *((int *)curr) == PVM_END_MEM_MARKER );
            if( *((int *)curr) != PVM_END_MEM_MARKER )
            {
                panic("No obj marker, no end of mem marker at 0x%X (arena %x-%x)", curr, pvm_object_space_start, pvm_object_space_end );
            }

            // Not an object, we are at the end of space and we need to create a new header here.
            if( size < (pvm_object_space_end - ((void *)curr) ) )
            {
                // Have free zero filled place - eat some

                // need it here to make sure pvm_alloc_next will work below.
                pvm_init_object_header(&(curr->_ah), size);

                // If allocation arena is not zero filled, make sure
                // it contains no object marker accidentally
                pvm_object_storage_t *nx = ((void *)curr)+size;
                if(
                   ( ((void *)nx)+sizeof(int) ) <= pvm_object_space_end
                  ) // Depends on the fact that object_start_marker is the first field!
                {
                    //nx->_ah.object_start_marker = 0;
                    *((int *)nx) = PVM_END_MEM_MARKER;
                }

                result = curr;
                break; // Ok
            }

            // Final non-inited piece is too small - search again from the start of mem pool
            curr = pvm_object_space_start;
            continue;

        }

        // Is an object.


        if(  PVM_OBJECT_AH_ALLOCATOR_FLAG_REFZERO & (curr->_ah.alloc_flags) )
        {
            hal_spin_lock( &refzero_spinlock );
            refzero_process_children( curr );
            hal_spin_unlock( &refzero_spinlock );
            // Supposed to be free here
        }

        if(  PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED & (curr->_ah.alloc_flags) )
        {
            // Is allocated? Go to the next one.
            curr = pvm_alloc_wrap_to_next_object(curr);
            continue;
        }

// FIXME - why off? Suspected to be wrong?
#if 0
        // Current object is free
        do {
            // See if the next is free too, combine spaces if so

            struct pvm_object_storage *next_obj = pvm_alloc_following_object(curr);
            if(next_obj != 0 && pvm_alloc_is_free_object(next_obj))
            {
                curr->_ah.exact_size += next_obj->_ah.exact_size;
                next_obj->_ah.object_start_marker = 0; // so it will not be found by memscan in gc
                continue; // Retry - maybe there are some more free objects follow
            }
        } while(0);
#endif
        // BUG: if last object (emptiness follows), must combine it with that emptiness

        // We are at the free object - check if size is ok

        if( curr->_ah.exact_size < size )
        {
            // Too small? Go to the next
            curr = pvm_alloc_wrap_to_next_object(curr);
            continue;
        }
//printf("allc free ");
printf("\r(^) ");

        // Ok, now cut off some piece
        unsigned int toomuch = curr->_ah.exact_size - size;
#if 1
// object division code is bad
        if( toomuch < 32 ) // don't break in too small pieces // BUG: constant - define
#endif
        {
            result = curr;
            break; // Take it as is
        }

        // Break down in two
        curr->_ah.exact_size = size;
        {
            struct pvm_object_storage *next_obj = pvm_alloc_following_object(curr);
            if(next_obj == 0)
            {
                result = curr;
                break;
            }
            pvm_init_object_header(&(next_obj->_ah), toomuch);
        }

        result = curr;
        break;

    }

    assert(result != 0);
    //pvm_object_space_alloc_current_position = pvm_alloc_wrap_to_next_object(result);
    pvm_object_space_alloc_current_position = result;
    return result;
}



// TODO include
extern int phantom_virtual_machine_threads_stopped;



struct pvm_object_storage *
pvm_object_alloc( unsigned int data_area_size )
{
    unsigned int size = sizeof(struct pvm_object_storage)+data_area_size;

    // BUG: dies after alloc of 5 mb in one piece
    //printf("alloc %d ", size);

    alloc_request_gc_pause++;
    hal_mutex_lock( &alloc_mutex );


#if 1

    struct pvm_object_storage * data;

    int ngc = 0;
    do {
        data = pvm_find(size);

        if(data)
            break;

        if(ngc-- <= 0)
            panic("out of mem looking for %d bytes", size);

        // We can't run GC from here cause it will reach semi-ready object
        // behind us and die. :(

#if GC_ENABLED
        alloc_request_gc_pause--; // In the mutex to prevent one more giveup by GC
        hal_mutex_unlock( &alloc_mutex );

        phantom_virtual_machine_threads_stopped++; // pretend we are stopped
printf("will gc... ");
        run_gc();
        phantom_virtual_machine_threads_stopped--;
printf("done gc\n");
        alloc_request_gc_pause++;
        hal_mutex_lock( &alloc_mutex );

//panic("mem dump after gc");

#endif
    } while(1);
#else
    struct pvm_object_storage *
        data = (struct pvm_object_storage *)  malloc(size);
    pvm_init_object_header(&(data->_ah), size);
#endif

    //if( data == 0 ) throw except( name, "out of memory" );
    data->_ah.alloc_flags = PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED;
    data->_ah.gc_flags = 0;
    data->_ah.refCount = 1;
    data->_da_size = data_area_size;

    // TODO remove it here - memory must be cleaned some other, more effective way
    memset( data->da, 0, data_area_size );

    alloc_request_gc_pause--; // In the mutex to prevent one more giveup by GC
    hal_mutex_unlock( &alloc_mutex );

    return data;
}



/*void object_delete( pvm_object_storage * o )
{
}*/

// -----------------------------------------------------------------------


/*inline struct pvm_object_storage *pvm_gc_next_object(struct pvm_object_storage *op)
{
    assert(op->_ah.object_start_marker == PVM_OBJECT_START_MARKER);
    void *o = (void *)op;
    o += op->_ah.exact_size;
    return (struct pvm_object_storage *)o;
}*/

#if 0
#define pvm_gc_next_object(op) \
({ \
    assert(op->_ah.object_start_marker == PVM_OBJECT_START_MARKER); \
    void *o = (void *)op; \
    o += op->_ah.exact_size; \
    o; \
})

#else

#define pvm_gc_next_object(op) \
({ \
    assert(op->_ah.object_start_marker == PVM_OBJECT_START_MARKER); \
    ((void *)op) + op->_ah.exact_size; \
})

#endif

// -----------------------------------------------------------------------
// Memory walk/check code
// -----------------------------------------------------------------------




/*
 *
 * Scan all the memory and check for inconsistencies.
 * Supposed to be called on kernel start before any
 * thread run.
 *
 */

int pvm_memcheck()
{
    long used = 0, free = 0, objects = 0;

    struct pvm_object_storage *curr = pvm_object_space_start;

    printf("Memcheck: checking object memory allocation consistency (at %x, %d bytes)\n", pvm_object_space_start, ((void *)pvm_object_space_start)-((void *)pvm_object_space_start) );

    while(((void *)curr) < pvm_object_space_end)
    {
        if(!pvm_alloc_is_object(curr))
        {
            int marker = (*((int *)curr)) == PVM_END_MEM_MARKER;
            // Not an object, we are at the end of space
            printf("Memcheck: end of objects at 0x%X (%d bytes used), marker is %s\n", curr, ((void *)curr)-((void *)pvm_object_space_start), marker ? "GOOD" : "BAD" );
            printf("Memcheck: %ld objects, memory: %ld used, %ld free\n", objects, used, free );
            return 0;
        }

        // Is an object.

        if( curr->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED )
            used += curr->_ah.exact_size;
        else
            free += curr->_ah.exact_size;

        objects++;

        curr = pvm_gc_next_object(curr);
    }

    printf("Memcheck: %ld objects, memory: %ld used, %ld free\n", objects, used, free );

    if(curr == pvm_object_space_end)
    {
        printf("Memcheck: reached exact arena end at 0x%X (%d bytes used)\n", curr, ((void *)curr)-((void *)pvm_object_space_start) );
        return 0;
    }

    printf("\n\n-----------------\nMemcheck ERROR: reached out of arena end at 0x%X (%d bytes size)\n-----------------\n\n", curr, ((void *)curr)-((void *)pvm_object_space_start) );
    return 1;
}






// -----------------------------------------------------------------------
// Ok, poor man's GC: mark/sweep only for now,
// TODO implement some more serious GC
// Not tested well!
// -----------------------------------------------------------------------


static void gc_bump_process_area(struct pvm_object_storage *curr);
static void gc_bump_process_children(
                                     struct pvm_object_storage *o,
                                     struct pvm_object_storage *curr
                                    );
static void gc_bump_scanmem(
                            char *mem, unsigned size,
                            struct pvm_object_storage *curr
                           );


static void scan_ds(struct pvm_object_storage *curr)
{
    //extern char _start_of_kernel[];
    //void *sod = _start_of_kernel;

    extern char _data_start__[];
    void *sod = _data_start__;

    extern char _bss_end__[];
    void *eod = &_bss_end__;

#if 0
    if( eod < sod )        		panic("GC: eod < sod");
    if( pvm_object_space_start < sod )  panic("GC: pvm_object_space_start < sod");
    if( eod < pvm_object_space_end )    panic("GC: eod < pvm_object_space_end");

    gc_bump_scanmem( sod, pvm_object_space_start-sod, curr );
    gc_bump_scanmem( pvm_object_space_end, eod-pvm_object_space_end, curr );
#else
    if( eod < sod )        		panic("GC: eod < sod");
    gc_bump_scanmem( sod, eod-sod, curr );
#endif
}

// gc_flags meanings

// Low 4 bits are generation
#define GC_FLAG_GENERATION_MASK 0x0F

// Upper bit is used to mark object that is accessible but
// which children is not processed yet and its generation is not
// yet upadated. Used only during 1st step (gc_bump_generations),
// must be clear all the other time
#define GC_FLAG_MARK 0x80

/*
 * The idea is very simple. We come through all the objects, starting
 * from roots, and increment generation number. On the second pass if
 * your generation is not current, you're dead. Second pass does not require
 * threads to be stopped.
 */



static int current_gc_generation;

#define GET_GC_GENERATION(ostorage) ((ostorage)->_ah.gc_flags & GC_FLAG_GENERATION_MASK)
#define SET_GC_GENERATION(ostorage, gen) ((ostorage)->_ah.gc_flags &= ~GC_FLAG_GENERATION_MASK, (ostorage)->_ah.gc_flags |= ((gen)& GC_FLAG_GENERATION_MASK) )

#define GC_GENERATION_MODULUS(gen) ((gen) & 0x0F)

void init_gc()
{
    // Get last generation number from root object
    current_gc_generation = GET_GC_GENERATION(get_root_object_storage());
}

static void gc_bump_generations(void);
static void gc_find_free(void);

// Not sure if we can intermix this with snaps. Won't for now.
//
// In fact, 2nd phase can be snapped as it is quite usual memory
// modification op and can run async to the mutators

void run_gc()
{
    hal_mutex_lock( &alloc_mutex );

    current_gc_generation = GC_GENERATION_MODULUS(current_gc_generation+1);
    // TODO disable snaps somehow - by doing this from snap thread?
    // Stop everybody
    //long start = time(0);
    phantom_snapper_wait_4_threads();
    // Update generations
//printf("GC bump generations, obj start at 0x%X\n", pvm_object_space_start);
    gc_bump_generations();
    // TODO reenable snaps

#if 1
    // Now check if we lost something
    //printf("GC scan DS\n");
    scan_ds(pvm_object_space_start);
    //printf("GC scan DS done, rescan data\n");
    gc_bump_generations();
    //printf("GC rescan data done\n");
#endif

//getchar();
    // Let 'em run
    phantom_snapper_reenable_threads();
    //printf("GC locked time %ld sec\n", time(0) - start);

    // Now free old stuff
//printf("GC find free\n");
    gc_find_free(); // Will temporarily give up alloc_mutex
//getchar();
    hal_mutex_unlock( &alloc_mutex );

}







static __inline__ int gc_is_actual( struct pvm_object_storage *o )
{
    return GET_GC_GENERATION(o) == current_gc_generation;
}

static __inline__ void gc_actualize( struct pvm_object_storage *o )
{
    SET_GC_GENERATION( o, current_gc_generation );
}


static __inline__ void gc_mark( struct pvm_object_storage *o )
{
    if( o != NULL ) o->_ah.gc_flags |= GC_FLAG_MARK;
}

static __inline__ void gc_unmark( struct pvm_object_storage *o )
{
    if( o != NULL ) o->_ah.gc_flags &= ~GC_FLAG_MARK;
}

static __inline__ int gc_is_marked( struct pvm_object_storage *o )
{
    return o->_ah.gc_flags & GC_FLAG_MARK;
}

static __inline__ void gc_marko( struct pvm_object o )
{
    gc_mark( o.data );
    gc_mark( o.interface );
}





int print_gc_free = 0;

// TODO: we can try and fix memory if smthng is broken?

static void gc_find_free(void)
{

    pvm_object_storage_t *curr = pvm_object_space_start;

    int gc_objects_collected = 0;
    int gc_bytes_collected = 0;

    int free_mem_size = 0;
    int free_max_piece = 0;

    //printf("GC: going to return free mem\n" );

    while(((void *)curr) < pvm_object_space_end)
    {
        /*if(alloc_request_gc_pause)
        {
            // BUG! Must mark current object somehow so that alloc won't
            // touch it - especially coalesce with prev or next

            hal_mutex_unlock( &alloc_mutex );
            hal_sleep_msec(10); // Let allocator go
            hal_mutex_lock( &alloc_mutex );
        }*/


        if(!pvm_alloc_is_object(curr))
        {
            assert( (*((int *)curr)) == PVM_END_MEM_MARKER );
            //int marker = (*((int *)curr)) == PVM_END_MEM_MARKER;
            //printf("GC: end of objects at 0x%X (%d bytes used), marker is %s\n", curr, ((void *)curr)-((void *)pvm_object_space_start), marker ? "GOOD" : "BAD" );

            /*if(!marker)
             panic("GC found memory inconsistency");*/
            // Not a panic since allocator may coalesce current object with other
            // till we slept - FIXME

            break;
        }

        // Check for prevoius and pre-previous generations. Others will
        // wait for 2 passes to go, I don't trust them.

        int gen = GET_GC_GENERATION(curr);
        if(
           (GC_GENERATION_MODULUS(gen+1) == current_gc_generation) ||
           (GC_GENERATION_MODULUS(gen+2) == current_gc_generation)
          )
        {
            if(curr->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED)
            {
                if(gc_is_marked(curr) )
                    panic("freeing marked!");

                gc_objects_collected++;
                gc_bytes_collected += curr->_ah.exact_size;
                if( 0 || print_gc_free )
                {
                    printf("Freeing %d bytes at %X ", curr->_ah.exact_size, curr );
                    //dumpo(curr);
                }
            }

            // Free it
            curr->_ah.alloc_flags &= (~PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED);
            // And make sure it has no refcount follower flag and won't be processed by refcount postprocessor
            curr->_ah.alloc_flags &= (~PVM_OBJECT_AH_ALLOCATOR_FLAG_REFZERO);
        }


        if(! (curr->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED) )
        {
            free_mem_size += curr->_ah.exact_size;
            if( curr->_ah.exact_size > free_max_piece)
                free_max_piece = curr->_ah.exact_size;
        }

        // Is an object.

        curr = pvm_gc_next_object(curr);
    }

    if(curr == pvm_object_space_end || ((*((int *)curr)) == PVM_END_MEM_MARKER) )
    {
        printf("\rGC OK, collected %6d objects, %9d bytes, %9dK free, %8dK max piece\n", gc_objects_collected,  gc_bytes_collected, free_mem_size/1024, free_max_piece/1024 );
        //printf("GC: reached exact arena end at 0x%X, %d bytes total, %d bytes freed\n", curr, ((void *)curr)-((void *)pvm_object_space_start), gc_bytes_collected );
        //printf("GC OK: at 0x%X, %d bytes total, %d bytes freed\n", curr, ((void *)curr)-((void *)pvm_object_space_start), gc_bytes_collected );
        return;
    }

    printf("GC ERROR: reached out of arena end at 0x%X, must be 0x%X (%d bytes size)\n", curr, pvm_object_space_end, ((void *)curr)-((void *)pvm_object_space_start) );

    //panic("GC found memory inconsistency");
    // Not a panic since allocator may coalesce current object with other
    // till we slept - FIXME

}

// GC helper area - keeps addresses of objects to visit - all of them
// are, of course, accessible (not garbage) - TEMP TODO static

//static int gc_area_size = 1024*1024;
#define gc_area_size (1024*1024)
static struct pvm_object_storage *gc_area[gc_area_size];

#if 1

int areapos = 0;

static void gc_area_add(struct pvm_object_storage *o)
{
    if(areapos >= gc_area_size)
        panic("out of area in GC");
    gc_area[areapos++] = o;
}

static struct pvm_object_storage * gc_area_get_and_remove()
{
    if( areapos <= 0 ) return 0;
    return gc_area[--areapos];
}



#else



/*static void gc_init_area()
{
    // empty yet
}

static void gc_kill_area()
{
    // empty yet
}*/

// This many slots to look checking for duplicates
#define HASH_LOOK_SLOTS 0xFFF

static int gc_area_hash(struct pvm_object_storage *o)
{
    unsigned int a = (unsigned  int)o;
    // 8 top bits. Mask for 1024*1024 is 0xFFFFF, so move left 12 bits
    return (((a >> 3) ^ (a >> 9 )) & 0xFF) << 12;
}

static void gc_area_add(struct pvm_object_storage *o)
{
    struct pvm_object_storage **put_pos = gc_area + gc_area_hash( o );

    //struct pvm_object_storage *start = gc_area_get_pos;
    struct pvm_object_storage **start = put_pos;

    int count = 0;

    while( 0 != *put_pos )
    {
        count++;
        put_pos++;
        if( put_pos >= gc_area+gc_area_size )
            put_pos = gc_area;
        if( put_pos == start )
            panic("out of area in GC");
    }

    *put_pos = o;

    while( count < HASH_LOOK_SLOTS )
    {
        count++;
        put_pos++;
        if( put_pos >= gc_area+gc_area_size )
            put_pos = gc_area;
        if( put_pos == start )
            break;

        // Duplicate found
        if( *put_pos == o )
            *put_pos = 0;

    }
}

/*
static void gc_area_remove(pvm_object_storage *o)
{
#error impl
}
*/

static struct pvm_object_storage **gc_area_get_pos  = 0;
static struct pvm_object_storage * gc_area_get_and_remove()
{
    if(
       (gc_area_get_pos < gc_area) ||
       (gc_area_get_pos > gc_area+gc_area_size)
      )
        gc_area_get_pos = gc_area;


    struct pvm_object_storage **start = gc_area_get_pos;


    while( 0 == *gc_area_get_pos )
    {
        gc_area_get_pos++;
        if( gc_area_get_pos >= gc_area+gc_area_size )
            gc_area_get_pos = gc_area;
        if( gc_area_get_pos == start )
            return 0;
    }

    struct pvm_object_storage *out = *gc_area_get_pos;
    *gc_area_get_pos = 0;

    return out;
}

#endif








/*
 *
 * We are moving through the objects up the address space and
 * walk through the all their outgoing links. If link is looking
 * down the memory we add it to area for later processing. If link
 * is looking up we just mark it as accessible but not processed
 * and will come to it later. On each step we first process current
 * object, then the links from area, and then come to next object in
 * address space.
 *
 */

void gc_bump_generations(void)
{
    gc_mark( pvm_object_space_start ); // Root is allways used

    if( ( sizeof(pvm_root) / sizeof(struct pvm_object)) != 26 )
        printf("GC Error: Check if all root object fields are marked in GC!\n");

    gc_marko( pvm_root.null_class );

    gc_marko( pvm_root.class_class );
    gc_marko( pvm_root.interface_class );
    gc_marko( pvm_root.code_class );

    gc_marko( pvm_root.int_class );
    gc_marko( pvm_root.string_class );

    gc_marko( pvm_root.array_class );
    gc_marko( pvm_root.page_class );

    gc_marko( pvm_root.thread_class );
    gc_marko( pvm_root.call_frame_class );
    gc_marko( pvm_root.istack_class );
    gc_marko( pvm_root.ostack_class );
    gc_marko( pvm_root.estack_class );

    gc_marko( pvm_root.boot_class );

    gc_marko( pvm_root.binary_class );
    gc_marko( pvm_root.bitmap_class );

    gc_marko( pvm_root.world_class );
    gc_marko( pvm_root.closure_class );

    gc_marko( pvm_root.null_object );
    gc_marko( pvm_root.sys_interface_object );
    gc_marko( pvm_root.class_loader );
    gc_marko( pvm_root.threads_list );
    gc_marko( pvm_root.windows_list );
    gc_marko( pvm_root.users_list );
    gc_marko( pvm_root.kernel_environment );
    gc_marko( pvm_root.os_entry );

    gc_roots_to_area(); // dynamically aded roots

    // FIXME Really need? Doesnt help. :(
    int i;
    for( i = 0; i < pvm_n_internal_classes; i++ )
    {
        gc_marko( pvm_internal_classes[i].class_object );
    }


    // Walk all the objects tree to see if they're used.

    struct pvm_object_storage *curr = pvm_object_space_start;

    while(((void *)curr) < pvm_object_space_end)
    {
        if( (*((int *)curr)) == PVM_END_MEM_MARKER )
            break;

        // TODO just move main loop CURR lower, then remove
        // from area and and mark everything in area that is
        // above the new CURR. Don't panic.
        if( !pvm_alloc_is_object( curr ) )
            panic("Not an object in GC bump");

        if( pvm_alloc_is_free_object( curr ) )
        {
            curr = pvm_gc_next_object(curr);
            continue;
        }

        if( gc_is_marked( curr ) )
        {
            gc_bump_process_children( curr, curr );
            gc_actualize( curr );
            gc_unmark( curr );
        }

        gc_bump_process_area( curr );

        curr = pvm_gc_next_object(curr);
    }

    gc_bump_process_area( curr );


}


static void gc_mark_or_add(
	struct pvm_object_storage *o,
        struct pvm_object_storage *curr,
        int must_be_marked
                                    )
{
    if( o == NULL || gc_is_actual( o ) )
        return;

    if( 1 && must_be_marked && !gc_is_marked( o ))
    {
        printf("GC: must be already mark_or_add(0x%X)'ed ", o);
        dumpo((int)o);
    }

    if( o > curr )
    {
        gc_mark( o );
        return;
    }
    if( o < curr )
    {
        gc_area_add( o );
    }

    //panic("gc_mark_or_add: o == curr (0x%X)", o);
//    printf("gc_mark_or_add: o == curr (0x%X)", o);
}


static void add_from_internal(struct pvm_object_storage * o, void *arg)
{
    struct pvm_object_storage *curr = arg;
    if( o != 0 )
        gc_mark_or_add( o, curr, 0 );
}


static int check_prev_object(unsigned int addr)
{
    void *op = (void *)addr;

    while( --op >= pvm_object_space_start )
    {
        if( ((pvm_object_storage_t *)op)->_ah.object_start_marker != PVM_OBJECT_START_MARKER )
            continue;

        unsigned int sz = ((pvm_object_storage_t *)op)->_ah.exact_size;

        return ( ((unsigned int)op)+sz == addr ) ? 0 : -1;
    }

    return -1; // No
}

static inline int addr_in_range(unsigned int addr)
{
    return ( (addr >= (unsigned int)pvm_object_space_start) && (addr < (unsigned int)pvm_object_space_end) );
}

static void gc_bump_scanmem(
	char *mem, unsigned size,
        struct pvm_object_storage *curr
                           )
{
    char *max = mem+size - sizeof(void *);

    for( ; mem < max; mem++ )
    {
        unsigned int iv = *((int *)mem);

        if(! addr_in_range(iv) )
            continue;

        struct pvm_object_storage *os = (void *)iv;
        //TODO ERR THIS STILL DOESNT PROVE THAT os IS OBJHECT ADDR!
        // Use this just to check above func!
        if( os->_ah.object_start_marker != PVM_OBJECT_START_MARKER )
            continue;

        if( ! (os->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED ) )
            continue;

        if( os->_ah.exact_size < sizeof(pvm_object_storage_t) )
            continue;

        if( !addr_in_range((int)os->_class.data) )
            continue;

        if( !addr_in_range((int)os->_class.interface) )
            continue;

        struct pvm_object_storage *no = pvm_gc_next_object(os);

        if( no->_ah.object_start_marker != PVM_OBJECT_START_MARKER )
            continue;

        // Already have, and the next check assumes it is not the first one
        // in the allocation arena
        if(iv == (unsigned int)pvm_object_space_start)
            continue;

        if( check_prev_object(iv) )
        {
            //printf("backcheck failed\n"); getchar();
            continue;
        }
        //printf("backcheck passed\n"); //getchar();

        gc_mark_or_add( os, curr, 1 );

    }

}


// TODO Might run out of area
// curr is where the main loop now: all objects above
// will be marked, all below will go to area
static void gc_bump_process_children(
	struct pvm_object_storage *o,
        struct pvm_object_storage *curr
                                    )
{
    gc_mark_or_add( o->_class.data, curr, 0 );
    gc_mark_or_add( o->_class.interface, curr, 0 );

    if( !(o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL) )
    {
        int i;

        for( i = 0; i < da_po_limit(o); i++ )
        {
            gc_mark_or_add( da_po_ptr(o->da)[i].data, curr, 0 );
            gc_mark_or_add( da_po_ptr(o->da)[i].interface, curr, 0 );
        }
        return;
    }

    // We're here if object is internal.

    // Fast skip for int and string and code - all of them
    // have no pointers out

    if(
       (o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING) ||
       (o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INT) ||
       (o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE)
      )
        return;

    // Now find and call class-specific function

    //struct pvm_object_storage *co = o->_class.data;
    gc_iterator_func_t  func = pvm_internal_classes[pvm_object_da( o->_class, class )->sys_table_id].iter;


    func( add_from_internal, o, curr );


    // well. this was added in desperation.
    // scan through all the da trying to treat everything as obj addr

    gc_bump_scanmem( o->da, o->_da_size, curr );
}

// Either mark or add to area all the children of the area's objects

static void gc_bump_process_area(struct pvm_object_storage *curr)
{

    struct pvm_object_storage *o;

    while ( 0 != (o = gc_area_get_and_remove()) )
    {
        gc_bump_process_children( o, curr );
        gc_actualize( o );
        gc_unmark( o );
    }

}


// These are temp. roots added from c code

#define gc_roots_size (1024*1024)
static struct pvm_object_storage *gc_roots[gc_roots_size];



int rootspos = 0;

void gc_root_add(struct pvm_object_storage *o)
{
    hal_spin_lock( &dynroots_lock );
    if(rootspos >= gc_roots_size)
    {
        hal_spin_unlock( &dynroots_lock );
        panic("out of roots space in GC");
    }
    gc_roots[rootspos++] = o;
    hal_spin_unlock( &dynroots_lock );
}

void gc_root_rm(struct pvm_object_storage *o)
{
    hal_spin_lock( &dynroots_lock );
    int i;
#if 1
    for( i = 0; i < gc_roots_size; i++ )
        if(gc_roots[i] == o)
        {
            gc_roots[i] = 0;
            break;
        }
#else
    // not tested
    for( i = 0; i < gc_roots_size; i++ )
        if(gc_roots[i] == o)
        {
            // swap with last and kill last then
            gc_roots[i] = gc_roots[rootspos-1];
            gc_roots[rootspos-1] = 0;
            rootspos--;
            break;
        }
#endif
    hal_spin_unlock( &dynroots_lock );
}

//static void gc_roots_to_area(struct pvm_object_storage *o)
static void gc_roots_to_area()
{
    int i;
    hal_spin_lock( &dynroots_lock );
    for( i = 0; i < gc_roots_size; i++ )
        gc_area_add( gc_roots[i] );
    hal_spin_unlock( &dynroots_lock );
}





// -----------------------------------------------------------------------
// Refcount processor.
// Takes object which is found to be nonreferenced (has PVM_OBJECT_AH_ALLOCATOR_FLAG_REFNONZERO flag)
// and processes all its children. Those with only one ref wil become marked with
// PVM_OBJECT_AH_ALLOCATOR_FLAG_REFNONZERO flag too.
// -----------------------------------------------------------------------

static void refzero_mark_or_add( pvm_object_storage_t * o );
static void refzero_add_from_internal(pvm_object_storage_t * o, void *arg);
static void do_refzero_process_children( pvm_object_storage_t *o );


static void refzero_process_children( pvm_object_storage_t *o )
{
    assert((o->_ah.alloc_flags) & PVM_OBJECT_AH_ALLOCATOR_FLAG_REFZERO);
    assert((o->_ah.alloc_flags) & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED);
    do_refzero_process_children( o );
    o->_ah.alloc_flags &= ~PVM_OBJECT_AH_ALLOCATOR_FLAG_REFZERO;
    o->_ah.alloc_flags &= ~PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED;
}

static void do_refzero_process_children( pvm_object_storage_t *o )
{

    //don't touch classes yet
    //refzero_mark_or_add( o->_class.data );
    //refzero_mark_or_add( o->_class.interface );

    if( !(o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL) )
    {
        int i;

        for( i = 0; i < da_po_limit(o); i++ )
        {
            refzero_mark_or_add( da_po_ptr(o->da)[i].data );
            //refzero_mark_or_add( da_po_ptr(o->da)[i].interface );
        }
        return;
    }

    // We're here if object is internal.

    // Fast skip for int and string and code - all of them
    // have no pointers out

    if(
       (o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING) ||
       (o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INT) ||
       (o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE)
      )
        return;

    // TEMP - skip classes and interfaces too. we must not reach them, in fact... how do we?
    if(
       (o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS) ||
       (o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE) ||
       (o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE)
      )
        return;


    // Now find and call class-specific function

    //struct pvm_object_storage *co = o->_class.data;
    gc_iterator_func_t  func = pvm_internal_classes[pvm_object_da( o->_class, class )->sys_table_id].iter;


    func( refzero_add_from_internal, o, 0 );


    // well. this was added in desperation.
    // scan through all the da trying to treat everything as obj addr

    //gc_bump_scanmem( o->da, o->_da_size, curr );
}

static void refzero_add_from_internal(pvm_object_storage_t * o, void *arg)
{
    //struct pvm_object_storage *curr = arg;

    // sanity check

    if( ((void*)o) < pvm_object_space_start || ((void*)o) >= pvm_object_space_end )
        panic("add from internal - out from object space");


    if( o != 0 )
        refzero_mark_or_add( o );
}

static void refzero_mark_or_add( pvm_object_storage_t * o )
{
    ref_dec_p(o);
    if(o->_ah.refCount == 0)
    {
        // FIXME must not be so in final OS, stack overflow possiblity!
        // TODO use some local pool too, instead of recursion
        refzero_process_children( o );
    }
}


