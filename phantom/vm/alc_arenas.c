/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * VM memory allocator: Arenas machinery
 * 
 * Each allocation is done out of one of the arenas.
 * All the persistent memory is a linked list of arenas.
 * Each arena is prefixed with quite usual Phantom object 
 * (though with no class and with a special flag )
 * which contains arena header.
 *
 * We keep set of arenas which are used for current allocations.
 * 
 * TODO on arena load - init mutex, on unload - deinit
 * 
**/


#define DEBUG_MSG_PREFIX "vm.alloc"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <vm/alloc.h>
#include <vm/object_flags.h>

#include <vm/internal_da.h>

#include <kernel/stats.h>
#include <kernel/page.h>
#include <kernel/vm.h>
#include <kernel/debug_graphical.h>

// ------------------------------------------------------------
// Private data
// ------------------------------------------------------------


// TODO for 64 bit mem we need more
#define N_PER_SIZE_ARENAS 12
static persistent_arena_t per_size_arena[N_PER_SIZE_ARENAS];

static persistent_arena_t curr_int_arena;     //< Integers
static persistent_arena_t curr_stack_arena;   //< Stack frames
static persistent_arena_t curr_static_arena;  //< Classes, code, etc
static persistent_arena_t curr_small_arena;   //< Less than 1K, but not one of the above
// TODO one for strings?


// ------------------------------------------------------------
// Prototypes for local code
// ------------------------------------------------------------


static void init_per_size_arena_flags( void );
static void alloc_find_arenas( void * _pvm_object_space_start, size_t size );
static int alloc_assert_is_arena( pvm_object_t a, void *start, void *end );
static persistent_arena_t *alloc_find_arena_by_flags( u_int32_t flags );

static void alloc_print_arena(persistent_arena_t *a);
static void alloc_print_arenas( void );

// ------------------------------------------------------------
// Init
// ------------------------------------------------------------


void alloc_init_arenas( void * _pvm_object_space_start, size_t size )
{
    init_per_size_arena_flags();

    if( is_object_storage_initialized() )
    {
        // We have snapshot, just find out what's in memory
        alloc_find_arenas( _pvm_object_space_start, size );
        alloc_print_arenas();
        return;
    }

    // Memory is empty, start fron scratch.

    // Decide on arena sizes and create arena header objects in memory
}


void init_per_size_arena_flags( void )
{
    int flags = 0x400; // 1K bytes

    int i;
    for( i = 0; i < N_PER_SIZE_ARENAS; i++ )
    {
        per_size_arena[i].flags = flags;
        flags <<= 1;
    }

}



static void alloc_find_arenas( void * start, size_t size )
{
    void *end = start+size;
    void *curr = start;

    memset( per_size_arena, 0, sizeof(per_size_arena) );
    memset( &curr_int_arena, 0, sizeof(curr_int_arena) );
    memset( &curr_stack_arena, 0, sizeof(curr_stack_arena) );
    memset( &curr_static_arena, 0, sizeof(curr_static_arena) );
    memset( &curr_small_arena, 0, sizeof(curr_small_arena) );

    while( curr < end )
    {
        alloc_assert_is_arena( curr, start, end );
        
        pvm_object_t a = curr;
        persistent_arena_t *da = (persistent_arena_t *) &(a->da);

        curr = curr + da->size; // Step

        LOG_INFO_( 1, "Found arena size %zd, free %zd, flags %x", da->size, da->free, da->flags );

        // Now decide if we need this arena. If so - load it.

        persistent_arena_t * our = alloc_find_arena_by_flags( da->flags );
        if( our == 0 )
        {
            LOG_ERROR( 1, "Don't have arena for flags %x", da->flags );
            continue;
        }

        if( our->base == 0 )
            alloc_load_arena( our, da );
        else if( our->base == da->base )
            continue;
        else if( alloc_is_better_arena( our, da ))
        {
            alloc_unload_arena( our );
            alloc_load_arena( our, da );
        }
    }
}





// ------------------------------------------------------------
// Sanity
// ------------------------------------------------------------

static int alloc_assert_is_arena( pvm_object_t a, void *start, void *end )
{
    assert( a->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_ARENA );
    assert( a >= start );
    assert( a < end );

    persistent_arena_t *da = (persistent_arena_t *) &(a->da);

    assert( da->arena_start_marker == PVM_ARENA_START_MARKER );
    assert( da->free < da->size );
    assert( da->largest < da->size );

}

// ------------------------------------------------------------
// Choose arena
// ------------------------------------------------------------


/// Get pointer to one of our arena records corresponding to given flags
static persistent_arena_t *alloc_find_arena_by_flags( u_int32_t flags )
{
    if( flags & PHANTOM_ARENA_FOR_INT )    return &curr_int_arena;
    if( flags & PHANTOM_ARENA_FOR_STACK )  return &curr_stack_arena;
    if( flags & PHANTOM_ARENA_FOR_SMALL )  return &curr_small_arena;
    if( flags & PHANTOM_ARENA_FOR_STATIC ) return &curr_static_arena;

    int i;
    for( i = 0; i < N_PER_SIZE_ARENAS; i++ )
    {
        if( per_size_arena[i].flags == flags )
            return per_size_arena + i;
    }

    return 0;
}


static persistent_arena_t *alloc_find_arena_by_data_size( size_t size )
{
    if( size == sizeof(struct pvm_object_storage) + sizeof(data_area_4_int) ) 
        return &curr_int_arena;

    if( size < 1024 ) 
        return &curr_small_arena;

    size_t ss = size / 1024;
    int i;
    for( i = 0; i < N_PER_SIZE_ARENAS; i++ )
    {
        if( size & ~0x1 ) // bits above lower are non-zero - out of size for this range
        {
            ss >>= 1;
            continue;
        }

        if( 0 == per_size_arena[i].base )
        {
            LOG_ERROR( 0, "Attempt to alloc %d bytes from unloaded arena %d", size, i );
            continue; // Use next, bigger one
        }

        LOG_FLOW( 1, "Alloc %d bytes: found arena %d, flags %x", size, i, per_size_arena[i].flags );
        return per_size_arena+i;

    }
}



// ------------------------------------------------------------
// Info
// ------------------------------------------------------------

static void alloc_print_arena(persistent_arena_t *a)
{
    LOG_INFO_( 1, "Arena flags %x, base %p, size %zd, free %zd, largest %zd", 
        a->flags, a->base, a->size, a->free, a->largest
         );
}

static void alloc_print_arenas( void )
{
    alloc_print_arena(  &curr_int_arena );
    alloc_print_arena( &curr_stack_arena );
    alloc_print_arena( &curr_small_arena );
    alloc_print_arena( &curr_static_arena );

    int i;
    for( i = 0; i < N_PER_SIZE_ARENAS; i++ )
        alloc_print_arena( per_size_arena+i );
}
