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
//#include "vm/root.h"
//#include "vm/exec.h"
#include "vm/object_flags.h"



//#define DEBUG_PRINT(a)
#define DEBUG_PRINT(a) printf((a))



static void init_free_object_header( pvm_object_storage_t *op, unsigned int size );

static void gc_roots_to_area();
static hal_spinlock_t dynroots_lock;

static hal_spinlock_t refzero_spinlock;
static void refzero_process_children( pvm_object_storage_t *o );

// taken if allocation is not possible now - by allocator and critical part of GC
static hal_mutex_t  alloc_mutex;
// allocator waits for GC to give him a chance
static volatile int     alloc_request_gc_pause = 0;


// Allocator and GC work in these bounds. NB! - pvm_object_space_end is OUT of arena
static void * pvm_object_space_start;
static void * pvm_object_space_end;


//
#define ARENAS 5
static void * start_a[ARENAS];
static void * end_a[ARENAS];
// Last position where allocator finished looking for objects
static void * curr_a[ARENAS];


// Names helper
static const char* name_a[ARENAS] = { "root, static", "stack", "int", "small", "large" };
// Partition
static int percent_a[ARENAS] = { 15, 15, 2, 48, 20 };


static inline int find_arena(unsigned int size, unsigned int flags, bool saturated)
{
    int arena = 0; // root|saturated|code|class|interface|Large - nearly constant

    if (flags & (PHANTOM_OBJECT_STORAGE_FLAG_IS_CALL_FRAME|PHANTOM_OBJECT_STORAGE_FLAG_IS_STACK_FRAME))
        arena = 1; //fast
    else if (flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INT)
        arena = 2; //small and fast
    else if (saturated)
        arena = 0; //never dies
    else if (flags & (PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE|PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS|PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE))
        arena = 0; //code|class|interface - never dies?
    else if (size < 1000)
        arena = 3; //small
    else // >1000
        arena = 4; //large

    return arena;
}

pvm_object_storage_t *get_root_object_storage() { return pvm_object_space_start; }

static void init_arenas( void * _pvm_object_space_start, unsigned int size )
{
    pvm_object_space_start = _pvm_object_space_start;
    pvm_object_space_end = pvm_object_space_start + size;

    void * cur = _pvm_object_space_start;
    int percent_100 = 0;
    int i;
    for( i = 0; i < ARENAS; i++) {
		start_a[i] = cur;
		curr_a[i] = cur;
		cur += (size / 400) * percent_a[i] * 4;  //align 4 bytes
		percent_100 += percent_a[i];
		end_a[i] = cur;
	}
	assert(percent_100 == 100); //check twice!
	end_a[ARENAS-1] = pvm_object_space_start + size; //to be exact
}


// Initialize the heap
void pvm_alloc_clear_mem()
{
    assert( pvm_object_space_start != 0 );
    int i;
    for( i = 0; i < ARENAS; i++) {
        init_free_object_header((pvm_object_storage_t *)start_a[i], end_a[i] - start_a[i]);
	}
}


// Initialize the heap, prepare
void pvm_alloc_init( void * _pvm_object_space_start, unsigned int size )
{
    assert(_pvm_object_space_start != 0);
    assert(size > 0);

    hal_spin_init( &refzero_spinlock );
    hal_spin_init( &dynroots_lock );

    init_arenas(_pvm_object_space_start, size);

    if( hal_mutex_init( &alloc_mutex ) )
        panic("Can't init allocator mutex");
}



static void init_object_header(pvm_object_storage_t *op, unsigned int size)
{
    assert( size >= sizeof(pvm_object_storage_t) );

    op->_ah.object_start_marker = PVM_OBJECT_START_MARKER;
    op->_ah.alloc_flags = PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED;
    op->_ah.gc_flags = 0;
    op->_ah.refCount = 1;
    op->_ah.exact_size = size;
}

static void init_free_object_header(pvm_object_storage_t *op, unsigned int size)
{
    assert( size >= sizeof(pvm_object_storage_t) );

    op->_ah.object_start_marker = PVM_OBJECT_START_MARKER;
    op->_ah.alloc_flags = PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE;
    op->_ah.gc_flags = 0;
    op->_ah.refCount = 0;
    op->_ah.exact_size = size;
}


#define PVM_MIN_FRAGMENT_SIZE  (sizeof(pvm_object_storage_t) + sizeof(int))      /* should be a minimal object size at least */


// returns allocated object
static pvm_object_storage_t *alloc_eat_some(pvm_object_storage_t *op, unsigned int size)
{
    assert( op->_ah.object_start_marker == PVM_OBJECT_START_MARKER );
    assert( op->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE );

    assert(op->_ah.exact_size >= size);
    unsigned int surplus = op->_ah.exact_size - size;
    if (surplus < PVM_MIN_FRAGMENT_SIZE) {
        // don't break in too small pieces
        init_object_header(op, op->_ah.exact_size);  //update alloc_flags
        return op;
    }

    init_object_header(op, size);  //update size and alloc_flags

    void *o = (void*)op + size;
    pvm_object_storage_t *opppa = (pvm_object_storage_t *)o;
    init_free_object_header(opppa, surplus);
    return op;
}


// try to collapse current with next objects until they are free
static void alloc_collapse_with_next_free(pvm_object_storage_t *op, unsigned int need_size, void * end)
{
    assert( op->_ah.object_start_marker == PVM_OBJECT_START_MARKER );
    assert( op->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE );

    unsigned int size = op->_ah.exact_size;
    do {
        void *o = (void *)op + size;
        pvm_object_storage_t *opppa = (pvm_object_storage_t *)o;
        if ( opppa->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE  &&  o < end ) {
            size += opppa->_ah.exact_size;
            init_free_object_header(op, size);  //update exact_size
            DEBUG_PRINT("^");
        } else {
            break;
        }
    } while(1);
}


// walk through
static pvm_object_storage_t *alloc_wrap_to_next_object(pvm_object_storage_t *op, void * start, void * end)
{
    assert(op->_ah.object_start_marker == PVM_OBJECT_START_MARKER);

    void *o = (void *)op + op->_ah.exact_size;

    if( o >= end )
    {
        assert(o <= end);
        o = start;
        DEBUG_PRINT("\n(alloc wrap)\n");
    }
    return (pvm_object_storage_t *)o;
}



static inline int pvm_alloc_is_object(pvm_object_storage_t *o)
{
    return o->_ah.object_start_marker == PVM_OBJECT_START_MARKER;
}

static inline int pvm_alloc_is_free_object(pvm_object_storage_t *o)
{
    return o->_ah.object_start_marker == PVM_OBJECT_START_MARKER
                 &&  o->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE ;
}

// maximal test:
bool pvm_object_is_allocated(pvm_object_storage_t *o)
{
    return o->_ah.object_start_marker == PVM_OBJECT_START_MARKER
            && o->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED
            //o->_ah.gc_flags == 0;
            && o->_ah.refCount > 0
            && o->_ah.exact_size >= ( o->_da_size + sizeof(pvm_object_storage_t) )
            && o->_ah.exact_size <  ( o->_da_size + sizeof(pvm_object_storage_t) + PVM_MIN_FRAGMENT_SIZE )
            && o->_ah.exact_size <= ( pvm_object_space_end - (void*)o ) ;
}

void pvm_object_is_allocated_assert(pvm_object_storage_t *o)
{
    assert( o->_ah.object_start_marker == PVM_OBJECT_START_MARKER );
    assert( o->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED );
    //o->_ah.gc_flags == 0;
    assert( o->_ah.refCount > 0 );
    assert( o->_ah.exact_size >= ( o->_da_size + sizeof(pvm_object_storage_t) ) );
    assert( o->_ah.exact_size <  ( o->_da_size + sizeof(pvm_object_storage_t) + PVM_MIN_FRAGMENT_SIZE ) );
    assert( o->_ah.exact_size <= (pvm_object_space_end - (void*)o) );
}




// Find a piece of mem of given or bigger size. Linear allocation.
static struct pvm_object_storage *pvm_find(unsigned int size, int arena)
{

#define CURR_POS  curr_a[arena]

//    static int rest_cnt = 0;
//
//    if(size < 64 && rest_cnt++ > 2000)
//    {
//        CURR_POS = start_a[arena];
//        rest_cnt = 0;
//    }
    // a kind of optimization above



    struct pvm_object_storage *result = 0;

    struct pvm_object_storage *start = start_a[arena];
    struct pvm_object_storage *end = end_a[arena];

    int wrapped = 0;
    struct pvm_object_storage *curr = CURR_POS;

    while(result == 0)
    {
        if( wrapped && ((void *)curr >= CURR_POS ) ) {
            return 0;
        }

        if( (void *)curr < CURR_POS )
            wrapped = 1;

        if(  PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED == curr->_ah.alloc_flags )
        {
            DEBUG_PRINT("a");
            // Is allocated? Go to the next one.
            curr = alloc_wrap_to_next_object(curr, start, end);
            continue;
        }

        if(  PVM_OBJECT_AH_ALLOCATOR_FLAG_REFZERO == curr->_ah.alloc_flags )
        {
            DEBUG_PRINT("(c)");

            hal_spin_lock( &refzero_spinlock );
            refzero_process_children( curr );
            hal_spin_unlock( &refzero_spinlock );
            // Supposed to be free here
        }

        // now free
        assert( PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE == curr->_ah.alloc_flags );

        if (curr->_ah.exact_size < size) {
            alloc_collapse_with_next_free(curr, size, end);
            // try again -
            if (curr->_ah.exact_size < size) {
                DEBUG_PRINT("|");
                curr = alloc_wrap_to_next_object(curr, start, end);
                continue;
            }
        }

        result = alloc_eat_some(curr, size);
        //break;
    }

    assert(result != 0);
    DEBUG_PRINT("+");

    CURR_POS = alloc_wrap_to_next_object(result, start, end);
    //CURR_POS = result;
    return result;
}




// TODO include
extern int phantom_virtual_machine_threads_stopped;



static pvm_object_storage_t * pool_alloc(unsigned int size, int arena)
{
    pvm_object_storage_t * data = 0;

    alloc_request_gc_pause++;
    hal_mutex_lock( &alloc_mutex );

    int ngc = 1;
    do {
        data = pvm_find(size, arena);

        if(data)
            break;

        if(ngc-- <= 0)
            break;

        printf("out of mem looking for %d bytes, arena %d \n", size, arena);

        // We can't run GC from here cause it will reach semi-ready object
        // behind us and die. :(

#if 1 //GC_ENABLED
        alloc_request_gc_pause--; // In the mutex to prevent one more giveup by GC
        //hal_mutex_unlock( &alloc_mutex );

        phantom_virtual_machine_threads_stopped++; // pretend we are stopped
printf("will gc... ");
        run_gc();
printf("done gc\n");
        phantom_virtual_machine_threads_stopped--;
        alloc_request_gc_pause++;
        //hal_mutex_lock( &alloc_mutex );

#endif //GC_ENABLED
    } while(1);

    alloc_request_gc_pause--; // In the mutex to prevent one more giveup by GC
    hal_mutex_unlock( &alloc_mutex );

    return data;
}


static inline void ref_saturate_p(pvm_object_storage_t *p);
void debug_catch_object(const char *msg, pvm_object_storage_t *p );
//allocation statistics:
#define max_stat_size 4096
static long created_o[ARENAS][max_stat_size+1];
static long created_large_o[ARENAS];
static long used_o[ARENAS][max_stat_size+1];
static long used_large_o[ARENAS];


pvm_object_storage_t * pvm_object_alloc( unsigned int data_area_size, unsigned int flags, bool saturated )
{
    unsigned int size = sizeof(pvm_object_storage_t) + data_area_size;
    size = (((size - 1)/ 4) + 1) * 4 ;  //align 4 bytes

    pvm_object_storage_t * data;

    int arena = find_arena(size, flags, saturated);


    data = pool_alloc(size, arena);

    if( data == 0 )
        panic("out of mem looking for %d bytes", size);

    data->_da_size = data_area_size;
    data->_flags = flags;
    if (saturated)
        ref_saturate_p(data);

    debug_catch_object("new", data);

    // TODO remove it here - memory must be cleaned some other, more effective way
    memset( data->da, 0, data_area_size );

    //stat
    if (size <= max_stat_size)
        created_o[arena][size]++;
    else
        created_large_o[arena]++;

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


static int memcheck_one(int i, void * start, void * end);
static void memcheck_print_histogram(int i);


/*
 *
 * Scan all the memory and check for inconsistencies.
 * Supposed to be called on kernel start before any
 * thread run.
 *
 */

int pvm_memcheck()
{
    int i;
    for( i = 0; i < ARENAS; i++)
    {
        printf("\n Arena #%d [%s] \n", i, name_a[i]);
        memcheck_one(i, start_a[i], end_a[i]);
    }

    return 0;
}


static int memcheck_one(int i, void * start, void * end)
{
    used_large_o[i] = 0; //reset
    int size;
    for( size = 0; size <= max_stat_size; size++)
        used_o[i][size] = 0; //reset

    long used = 0, free = 0, objects = 0, largest = 0;

    struct pvm_object_storage *curr = start;

    printf("Memcheck: checking object memory allocation consistency (at %x, %d bytes)\n", start, (end - start) );

    while(((void *)curr) < end)
    {
        if(!pvm_alloc_is_object(curr))
        {
            //printf("Memcheck: %ld objects, memory: %ld used, %ld free, %ld largest\n", objects, used, free, largest );
            //return 0;
            printf("Memcheck: not an object at 0x%X\n", curr);
            break;
        }

        // Is an object.

        if( curr->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED )
        {
            if(!pvm_object_is_allocated(curr))  //more tests
            {
                printf("Memcheck: corrupted allocated object at 0x%X\n", curr);
                break;
            }
            if (curr->_ah.exact_size <= max_stat_size)
                used_o[i][curr->_ah.exact_size]++;
            else
                used_large_o[i]++;

            used += curr->_ah.exact_size;
            objects++;
        }
        else
        {
            free += curr->_ah.exact_size;
            if( curr->_ah.exact_size > largest )
                largest = curr->_ah.exact_size;
        }


        curr = pvm_gc_next_object(curr);
    }

    //printf("Memcheck: %ld objects, memory: %ld used, %ld free\n", objects, used, free );
    printf("Memcheck: %ld objects, memory: %ld used, %ld free, %ld largest\n", objects, used, free, largest );

    if((void *)curr == end)
    {
        printf("Memcheck: reached exact arena end at 0x%X (%d bytes used)\n", curr, ((void *)curr) - start );
        memcheck_print_histogram(i);
        return 0;
    }

    printf("\n\n-----------------\nMemcheck ERROR: reached out of arena end at 0x%X (%d bytes size)\n-----------------\n\n", curr, ((void *)curr) - start );
    return 1;
}


static void memcheck_print_histogram(int arena)
{
    if (arena == 0) return; //nothing interesting

    if (created_large_o[arena] > 0)
        printf(" large objects: now used %d, was allocated %d\n", used_large_o[arena], created_large_o[arena]);

    printf(" small objects: size, now used, was allocated\n");
    int size;
    for( size = 0; size <= max_stat_size; size++)
    {
        if(created_o[arena][size] > 0)
            printf("   %6d %6d %12d \n", size, used_o[arena][size], created_o[arena][size]);
    }
}





// -----------------------------------------------------------------------
// Ok, poor man's GC: mark/sweep only for now,
// TODO implement some more serious GC
// -----------------------------------------------------------------------


void init_gc() {};

static void free_unmarked();
static void mark_tree(pvm_object_storage_t * root);
static void gc_bump_process_children(pvm_object_storage_t *o);


void run_gc()
{
    pvm_memcheck();  // visualization
    DEBUG_PRINT("gc started...\n");


    // First pass - tree walk, recursively (for now).
    //
    // Root always used. All other objects, including pvm_root and pvm_root.threads_list, should be reached from root...
    pvm_object_storage_t *root = get_root_object_storage();

    mark_tree( root ); // Root is allways used


    // Second pass - linear walk to free unused objects
    //
    free_unmarked();

    DEBUG_PRINT("gc finished!\n");
    pvm_memcheck();  // visualization
}


static void free_unmarked()
{
    void * curr;
    for( curr = pvm_object_space_start; curr < pvm_object_space_end ; curr += ((pvm_object_storage_t *)curr)->_ah.exact_size )
    {
        pvm_object_storage_t * p = (pvm_object_storage_t *)curr;

        if (p->_ah.gc_flags != 0)
            p->_ah.gc_flags = 0;  // clear marked
        else
            if ( p->_ah.refCount > 0 )
            {
                debug_catch_object("gc", p);
                p->_ah.refCount = 0;  // free now
                p->_ah.alloc_flags = PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE; // free now
            }
    }
}

static inline void mark(pvm_object_storage_t * p)
{
    if (p != 0)
       p->_ah.gc_flags = 1;
}

static void mark_tree(pvm_object_storage_t * root)
{
    if (root == 0)
        return;

    if (root->_ah.gc_flags == 1)
        return; // already done

    mark(root);
    gc_bump_process_children(root);
}

static void mark_tree_o(struct pvm_object o)
{
    mark_tree(o.data);
    mark_tree(o.interface);
}

static void add_from_internal(pvm_object_storage_t * o, void *arg)
{
    mark_tree( o );
}

static void gc_bump_process_children(pvm_object_storage_t *o)
{
    mark_tree_o( o->_class );

    // Fast skip for int and string and code - all of them
    // have no pointers out
    if(
       (o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING) ||
       (o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INT) ||
       (o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE)
      )
        return;

    if( !(o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL) )
    {
        int i;

        for( i = 0; i < da_po_limit(o); i++ )
        {
            mark_tree_o( da_po_ptr(o->da)[i] );
        }
        return;
    }
    // We're here if object is internal.


    // Now find and call class-specific function

    gc_iterator_func_t  func = pvm_internal_classes[pvm_object_da( o->_class, class )->sys_table_id].iter;

    func( add_from_internal, o, 0 );
}







// -----------------------------------------------------------------------
// Refcount processor.
// Takes object which is found to be nonreferenced (has PVM_OBJECT_AH_ALLOCATOR_FLAG_REFZERO flag)
// and processes all its children. Those with only one ref will become marked with
// PVM_OBJECT_AH_ALLOCATOR_FLAG_REFZERO flag too.
// -----------------------------------------------------------------------

static void refzero_mark_or_add( pvm_object_storage_t * o );
static void refzero_add_from_internal(pvm_object_storage_t * o, void *arg);
static void do_refzero_process_children( pvm_object_storage_t *o );
void ref_free_stackframe( pvm_object_storage_t *o ); // TODO to header!


static void refzero_process_children( pvm_object_storage_t *o )
{
    assert( o->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_REFZERO );
    do_refzero_process_children( o );

    o->_ah.alloc_flags = PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE;
    debug_catch_object("del", o);
    DEBUG_PRINT("x");
}

static void do_refzero_process_children( pvm_object_storage_t *o )
{

    if( !(o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL) )
    {
        int i;

        for( i = 0; i < da_po_limit(o); i++ )
        {
            ref_dec_o( da_po_ptr(o->da)[i] );
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

    if(o->_flags & (PHANTOM_OBJECT_STORAGE_FLAG_IS_CALL_FRAME) )
    {
        /*
         * NB! It is FORBIDDEN to follow references in stack frame manually.
         *
         * Stack frame points to stacks, and stacks are double linked and do not
         * support regular refcount. And can't be freed by regular refcount.
         * So due to said above and the total ownership of stacks by stack
         * frame we are freeing them manually when stack frame is freed by
         * refcount code.
         *
         */
        ref_free_stackframe(o);
        return;
    }

    if(o->_flags & (PHANTOM_OBJECT_STORAGE_FLAG_IS_THREAD) )
    {
        /*
         * NB! It is FORBIDDEN to follow references in stack frame manually.
         *
         * Special version of thread iter function for refcount - avoid following stack frames.
         *
         */
        pvm_gc_iter_thread_1( add_from_internal, o, 0 );
        return;
    }


    // Now find and call class-specific function

    //struct pvm_object_storage *co = o->_class.data;
    gc_iterator_func_t  func = pvm_internal_classes[pvm_object_da( o->_class, class )->sys_table_id].iter;

    // Iterate over reference-type fields of internal-class objects, calling refzero_add_from_internal for each such field.
    func( refzero_add_from_internal, o, 0 );


    //don't touch classes yet
    //ref_dec_o( o->_class );  // Why?


    // well. this was added in desperation.
    // scan through all the da trying to treat everything as obj addr

    //gc_bump_scanmem( o->da, o->_da_size, curr );
}

static inline void ref_dec_p(pvm_object_storage_t *p);

static void refzero_add_from_internal(pvm_object_storage_t * o, void *arg)
{
    if ( o == 0 )
       return;

    DEBUG_PRINT("Ku");
    ref_dec_p( o );
}




/*-----------------------------------------------------------------------------------------*/

// used by   ref_dec_p()
static inline void ref_dec_proccess_zero(pvm_object_storage_t *p)
{
    assert( p->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED );
    assert( p->_ah.refCount == 0 );

    // free immediately if no children
    if ((p->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL) &&
        (p->_flags & (PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING |
                       PHANTOM_OBJECT_STORAGE_FLAG_IS_INT |
                       PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE
                    ))) {
        p->_ah.alloc_flags = PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE;
        debug_catch_object("del", p);
        DEBUG_PRINT("-");
    }
    else {
        // postpone for delayed inspection (bug or feature?)
        p->_ah.alloc_flags = PVM_OBJECT_AH_ALLOCATOR_FLAG_REFZERO;
        DEBUG_PRINT("(X)");

        // FIXME must not be so in final OS, stack overflow possiblity!
        // TODO use some local pool too, instead of recursion

        // or, alternatively, just comment out the following lines to postpone deallocation to future
        hal_mutex_lock( &alloc_mutex );
        refzero_process_children( p );
        hal_mutex_unlock( &alloc_mutex );
    }
}


void debug_catch_object(const char *msg, pvm_object_storage_t *p )
{
    // Can be used to trace some specific object's access
    //if( p != 0x7A9F3527 )
    //if( !strcmp(msg, "gc") )
        return;
    printf("touch %s 0x%X, refcnt = %d, size = %d da_size = %d ", msg, p, p->_ah.refCount, p->_ah.exact_size, p->_da_size);

    print_object_flags(p);
    //getchar();
    printf("\n"); // for GDB to break here
}

static inline void ref_dec_p(pvm_object_storage_t *p)
{
    debug_catch_object("--", p);

    assert( p->_ah.object_start_marker == PVM_OBJECT_START_MARKER );
    assert( p->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED );

    //if(p->_ah.refCount <= 0) {
    /*if( p->_ah.alloc_flags != PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED ) {
       //DEBUG_PRINT("Y");
        printf(" %d", p->_ah.refCount );
        printf(" @ 0x%X", p); getchar();
    }*/
    assert( p->_ah.refCount > 0 );

    if(p->_ah.refCount < INT_MAX) // Do we really need this check? Sure, we see many decrements for saturated objects!
    {
        if( 0 == ( --(p->_ah.refCount) ) )
            ref_dec_proccess_zero(p);
    }
}

static inline void ref_inc_p(pvm_object_storage_t *p)
{
    debug_catch_object("++", p);

    assert( p->_ah.object_start_marker == PVM_OBJECT_START_MARKER );
    assert( p->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED );
    assert( p->_ah.refCount > 0 );

    if( p->_ah.refCount <= 0 )
    {
        panic("p->_ah.refCount <= 0: 0x%X", p);
    }

    if( p->_ah.refCount < INT_MAX )
        (p->_ah.refCount)++;
}


//external calls:

// Make sure this object won't be deleted with refcount dec
// used on sys global objects
static inline void ref_saturate_p(pvm_object_storage_t *p)
{
    debug_catch_object("!!", p);

    assert( p->_ah.object_start_marker == PVM_OBJECT_START_MARKER );
    assert( p->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED );
    assert( p->_ah.refCount > 0 );

    p->_ah.refCount = INT_MAX;
}

void ref_saturate_o(pvm_object_t o)
{
    if(!(o.data)) return;
    ref_saturate_p(o.data);
}
void ref_dec_o(pvm_object_t o)
{
    ref_dec_p(o.data);
}
void ref_inc_o(pvm_object_t o)
{
    ref_inc_p(o.data);
}


