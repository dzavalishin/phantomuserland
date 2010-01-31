/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/



//#include <kernel/snap_sync.h>


#include "vm/alloc.h"
#include "vm/internal.h"
#include "vm/object_flags.h"



#define debug_memory_leaks 0
#define debug_allocation 0


#define DEBUG_PRINT(a)   if (debug_allocation) printf(a)
#define DEBUG_PRINT1(a,b)  if (debug_allocation) printf(a,b)




// -----------------------------------------------------------------------
// Collect cycles --  refcounter-based full GC
// see Bacon algorithm (US Patent number 6879991, issued April 12, 2005) or (US Patent number 7216136 issued 8 May 2007)
// TODO: not implemented,
// Need persistent cycles_root_buffer not collected by usual gc/refcount - new internal object type?

static void cycle_root_buffer_add_candidate(pvm_object_storage_t *p)
{
}
static void cycle_root_buffer_rm_candidate(pvm_object_storage_t *p)
{
}
static void cycle_root_buffer_clear()
{
    //just set size to zero, so regular GC will ignore it gracefully
}
void gc_collect_cycles()
{
}






// -----------------------------------------------------------------------
// Ok, poor man's GC:  stop-the-world mark/sweep
// TODO implement some more serious GC
// -----------------------------------------------------------------------


//static void init_gc() {};

static int free_unmarked();
static void mark_tree(pvm_object_storage_t * root);

//typedef void (*gc_iterator_call_t)( struct pvm_object o, void *arg );
//typedef void (*gc_iterator_func_t)( gc_iterator_call_t func, struct pvm_object_storage * os, void *arg );

static void mark_tree_o(pvm_object_t o, void *arg);
static void gc_process_children(gc_iterator_call_t f, pvm_object_storage_t *p, void *arg);

void debug_catch_object(const char *msg, pvm_object_storage_t *p );


//// TODO include
//extern int phantom_virtual_machine_threads_stopped;
static volatile int  gc_n_run = 0;
static unsigned char  gc_flags_last_generation = 0;

void run_gc()
{
    int my_run = gc_n_run;

    hal_mutex_lock( &alloc_mutex );

    if (my_run != gc_n_run) // lock acquired when concurrent gc run finished
        { hal_mutex_unlock( &alloc_mutex ); return; }
    gc_n_run++;

    gc_flags_last_generation++; // bump generation
    if (gc_flags_last_generation == 0)  gc_flags_last_generation++;  // != 0 'cause allocation reset gc_flags to zero

    //phantom_virtual_machine_threads_stopped++; // pretend we are stopped
    //TODO: refine sinchronization

    if (debug_memory_leaks) pvm_memcheck();  // visualization
    if (debug_memory_leaks) printf("gc started...  ");


    cycle_root_buffer_clear(); // so two types of gc could coexists


    // First pass - tree walk, mark visited.
    //
    // Root is always used. All other objects, including pvm_root and pvm_root.threads_list, should be reached from root...
    mark_tree( get_root_object_storage() );


    // Second pass - linear walk to free unused objects.
    //
    int freed = free_unmarked();

    if ( freed > 0 )
       printf("\ngc: %i objects freed\n", freed);

    if (debug_memory_leaks) printf("gc finished!\n");
    if (debug_memory_leaks) pvm_memcheck();  // visualization

    //TODO refine sinchronization
    //phantom_virtual_machine_threads_stopped--;
    hal_mutex_unlock( &alloc_mutex );
}


static int free_unmarked()
{
    void * start = get_pvm_object_space_start();
    void * end = get_pvm_object_space_end();
    void * curr;
    int freed = 0;
    for( curr = start; curr < end ; curr += ((pvm_object_storage_t *)curr)->_ah.exact_size )
    {
        pvm_object_storage_t * p = (pvm_object_storage_t *)curr;
        assert( p->_ah.object_start_marker == PVM_OBJECT_START_MARKER );

        if ( (p->_ah.gc_flags != gc_flags_last_generation) && ( p->_ah.alloc_flags != PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE ) )  //touch not accessed but allocated objects
        {
            if( p->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_FINALIZER )
            {
                // based on the assumption that finalizer is only valid for some internal childfree objects - is it correct?
                gc_finalizer_func_t  func = pvm_internal_classes[pvm_object_da( p->_class, class )->sys_table_id].finalizer;
                if (func != 0) func(p);
            }

            freed++;
            debug_catch_object("gc", p);
            p->_ah.refCount = 0;  // free now
            p->_ah.alloc_flags = PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE; // free now
        }
    }
    return freed;
}

static void mark_tree(pvm_object_storage_t * p)
{
    p->_ah.gc_flags = gc_flags_last_generation;  // set


    assert( p->_ah.object_start_marker == PVM_OBJECT_START_MARKER );
    assert( p->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED );


    // Fast skip if no children -
    if( !(p->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE) )
    {
        gc_process_children( mark_tree_o, p, 0 /*unused cookie*/ );
    }
}

static void mark_tree_o(pvm_object_t o, void *arg)
{
    if(o.data == 0) // Don't try to process null objects
        return;

    if (o.data->_ah.gc_flags != gc_flags_last_generation)  mark_tree( o.data );
    if (o.interface->_ah.gc_flags != gc_flags_last_generation)  mark_tree( o.interface );
}


static void gc_process_children(gc_iterator_call_t f, pvm_object_storage_t *p, void *arg)
{
    f( p->_class, arg );

    // Fast skip if no children - done!
    //if( p->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE )
    //    return;

    // plain non internal objects -
    if( !(p->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL) )
    {
        int i;

        for( i = 0; i < da_po_limit(p); i++ )
        {
            f( da_po_ptr(p->da)[i], arg );
        }
        return;
    }

    // We're here if object is internal.

    // Now find and call class-specific function: pvm_gc_iter_*

    gc_iterator_func_t  func = pvm_internal_classes[pvm_object_da( p->_class, class )->sys_table_id].iter;

    func( f, p, arg );
}







// -----------------------------------------------------------------------
// Refcount processor.
// Takes object which is found to be nonreferenced (has PVM_OBJECT_AH_ALLOCATOR_FLAG_REFZERO flag)
// and processes all its children. Those with only one ref will become marked with
// PVM_OBJECT_AH_ALLOCATOR_FLAG_REFZERO flag too.
// -----------------------------------------------------------------------

static void refzero_add_from_internal(pvm_object_t o, void *arg);
static void do_refzero_process_children( pvm_object_storage_t *p );



void refzero_process_children( pvm_object_storage_t *p )
{
    assert( p->_ah.alloc_flags != PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE );
    assert( p->_ah.refCount == 0 );

    do_refzero_process_children( p );

    if ( p->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_IN_BUFFER )
        cycle_root_buffer_rm_candidate( p );

    p->_ah.alloc_flags = PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE;

    debug_catch_object("del", p);
    DEBUG_PRINT("x");
}

static void do_refzero_process_children( pvm_object_storage_t *p )
{
    // Fast skip if no children - done!
    //if( o->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE )
    //    return;

    // plain non internal objects -
    if( !(p->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL) )
    {
        int i;

        for( i = 0; i < da_po_limit(p); i++ )
        {
            ref_dec_o( da_po_ptr(p->da)[i] );
        }
        //don't touch classes yet
        //ref_dec_o( p->_class );  // Why? a kind of mismatch in the compiler, in opcode_os_save8

        return;
    }

    // We're here if object is internal.


    // TEMP - skip classes and interfaces too. we must not reach them, in fact... how do we?
    if(
       (p->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS) ||
       (p->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE) ||
       (p->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE)
      )
        //panic("");
        return;


    // Now find and call class-specific function: pvm_gc_iter_*

    gc_iterator_func_t  func = pvm_internal_classes[pvm_object_da( p->_class, class )->sys_table_id].iter;

    func( refzero_add_from_internal, p, 0 );


    //don't touch classes yet
    //ref_dec_o( p->_class );  // Why? Internal class objects are saturated for now.
}


static void refzero_add_from_internal(pvm_object_t o, void *arg)
{
    if(o.data == 0) // Don't try to process null objects
       return;

    ref_dec_o( o );
}




/*-----------------------------------------------------------------------------------------*/

#define RECURSE_REF_DEC 1

// used by   ref_dec_p()
static void ref_dec_proccess_zero(pvm_object_storage_t *p)
{
    assert( p->_ah.alloc_flags != PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE );
    assert( p->_ah.refCount == 0 );
    assert( !(p->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE) );

#if RECURSE_REF_DEC
    // FIXME must not be so in final OS, stack overflow possiblity!
    // TODO use some local pool too, instead of recursion
    refzero_process_children( p );
#else
    // postpone for delayed inspection (bug or feature?)
    DEBUG_PRINT("(X)");
    p->_ah.alloc_flags |= PVM_OBJECT_AH_ALLOCATOR_FLAG_REFZERO; //beware of  PVM_OBJECT_AH_ALLOCATOR_FLAG_IN_BUFFER
    p->_ah.alloc_flags &= ~PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED; //beware of  PVM_OBJECT_AH_ALLOCATOR_FLAG_IN_BUFFER
#endif
}



void debug_catch_object(const char *msg, pvm_object_storage_t *p )
{
    // Can be used to trace some specific object's access
    //if( p != (void *)0x7acbe56c )
    if( p != (void *)0x7acbd0e8 )
    //if( 0 != strncmp(msg, "gc", 2) || !debug_memory_leaks )
    //if( !(p->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE) )
        return;
    printf("touch %s 0x%X, refcnt = %d, size = %d da_size = %d ", msg, p, p->_ah.refCount, p->_ah.exact_size, p->_da_size);

    print_object_flags(p);
    //dumpo(p);
    //getchar();
    printf("\n"); // for GDB to break here
}

static inline void ref_dec_p(pvm_object_storage_t *p)
{
    debug_catch_object("--", p);

    assert( p->_ah.object_start_marker == PVM_OBJECT_START_MARKER );
    assert( p->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED );
    assert( p->_ah.refCount > 0 );

    //if(p->_ah.refCount <= 0) {
    /*if( !(p->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED) ) {
       //DEBUG_PRINT("Y");
        printf(" %d", p->_ah.refCount );
        printf(" @ 0x%X", p); getchar();
    }*/

    if(p->_ah.refCount < INT_MAX) // Do we really need this check? Sure, we see many decrements for saturated objects!
    {
        if( 0 == ( --(p->_ah.refCount) ) )
        {
            // Fast way if no children
            if( p->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE )
            {
                if( p->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_FINALIZER )
                {
                    // based on the assumption that finalizer is only valid for some internal childfree objects - is it correct?
                    gc_finalizer_func_t  func = pvm_internal_classes[pvm_object_da( p->_class, class )->sys_table_id].finalizer;
                    if (func != 0) func(p);
                }

                p->_ah.alloc_flags = PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE;
                debug_catch_object("del", p);
                DEBUG_PRINT("-");
            } else
                ref_dec_proccess_zero(p);
        }

        // if we decrement refcount and stil above zero - mark an object as potential cycle root;
        // and internal objects can't be a cycle root (sic!)
        else if ( !(p->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL) )
        {
            if ( !(p->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_IN_BUFFER) )
            {
                cycle_root_buffer_add_candidate(p);
                p->_ah.alloc_flags |= PVM_OBJECT_AH_ALLOCATOR_FLAG_IN_BUFFER ;
            }
            p->_ah.alloc_flags |= PVM_OBJECT_AH_ALLOCATOR_FLAG_WENT_DOWN ;  // set down flag
        }
    }
}

static inline void ref_inc_p(pvm_object_storage_t *p)
{
    debug_catch_object("++", p);

    assert( p->_ah.object_start_marker == PVM_OBJECT_START_MARKER );
    assert( p->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED );
    assert( p->_ah.refCount > 0 );

    //if( p->_ah.refCount <= 0 )
    //{
    //    panic("p->_ah.refCount <= 0: 0x%X", p);
    //}

    if( p->_ah.refCount < INT_MAX )
    {
        (p->_ah.refCount)++;

        if ( p->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_IN_BUFFER )
            p->_ah.alloc_flags &= ~PVM_OBJECT_AH_ALLOCATOR_FLAG_WENT_DOWN ;  //clear down flag
    }
}


// Make sure this object won't be deleted with refcount dec
// used on sys global objects
void ref_saturate_p(pvm_object_storage_t *p)
{
    debug_catch_object("!!", p);

    // Saturated object can't be a loop collection candidate. Can it?
    if ( p->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_IN_BUFFER )
        cycle_root_buffer_rm_candidate( p );

    p->_ah.alloc_flags &= ~PVM_OBJECT_AH_ALLOCATOR_FLAG_IN_BUFFER;

    assert( p->_ah.object_start_marker == PVM_OBJECT_START_MARKER );
    // [dz] was assert( p->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED );
    // But fails if object had PVM_OBJECT_AH_ALLOCATOR_FLAG_IN_BUFFER
    // TODO Check if PVM_OBJECT_AH_ALLOCATOR_FLAG_IN_BUFFER has to be handled
    // here
    assert( p->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED );
    assert( p->_ah.refCount > 0 );

    p->_ah.refCount = INT_MAX;
}


//external calls:
void ref_saturate_o(pvm_object_t o)
{
    if(!(o.data)) return;
    ref_saturate_p(o.data);
}
pvm_object_t  ref_dec_o(pvm_object_t o)
{
    // Interface is never refcounted! (But why?)
    ref_dec_p(o.data);
    return o;
}
pvm_object_t  ref_inc_o(pvm_object_t o)
{
    ref_inc_p(o.data);
    return o;
}


