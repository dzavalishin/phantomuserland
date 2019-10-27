/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Snapshot based garbage collection - NOT FINISHED
 *
**/



#include <kernel/snap_sync.h>

#define DEBUG_MSG_PREFIX "vm.gc.long"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <vm/alloc.h>
#include <vm/internal.h>
#include <vm/object_flags.h>

#include <kernel/stats.h>
#include <kernel/atomic.h>

static void long_gc_build_map(size_t address_range);

static size_t lgc_mem_start;
static size_t lgc_mem_end;
static size_t lgc_mem_size;

/**
 * 
 * @brief Do snapshot-based GC cycle. Really slow!
 * 
 * This is a (very) long term Garbage Collection
 * subsystem.
 * 
 * General idea: scan through last snapshot, do
 * classic mark/sweep garbage collection, free
 * found garbage in running OS memory image.
 * 
 * @note Can't run with other GC running on actual memory.
 * 
 * If refcount (or any other GC) is running it is possible
 * that in memory GC will free object, other object is created
 * in its place and finally long GC will free new (wrong)
 * object.
 * 
 * So during our work refcount GC must be stopped and any
 * pending refdec requests cleared.
 * 
 * It is, thought, possible to split memory range - let 
 * refcount GC work on new arenas and long GC to work on
 * older objects.
 * 
 * In this case long GC must scan all memory, but release
 * objects only in given range.
 * 
 * @todo Add parameters for release memory range.
 * 
**/

void pvm_snapshot_gc( void )
{
// Get actual address space size
    lgc_mem_start;
    lgc_mem_end;
    lgc_mem_size = lgc_mem_end - lgc_mem_start;

// Determine amount of memory we need for object map
#define ASSUME_OBJECTS_PER_KB (1024/50)

// Allocate data structures needed
    long_gc_build_map();

//
// ROOTS PHASE
//
// Mark all objects that are obvious roots.
// Actually we can do it in scan phase.
//
    long_gc_roots_phase();

//
// SCAN PHASE
//
// Find all objects in snapshot
//
    long_gc_scan_phase();

//
// MARK PHASE 
//
// Process map - mark rechable elements
// Try to be local - each el has 3 bits - 
// visited, marked and childfree.
// Try to process all unvisited marked non-
// childfree objects around one address
//
    long_gc_mark_phase();

// SWEEP PHASE
    long_gc_sweep_phase();

}

#define ELEM_PER_PAGE 1024
// Range[0]*1024 must be bigger than mem range we need
// Range[2] is memory range per one slot of 3rd level map
static size_t long_gc_map_level_range[3];

pvm_object_t     long_gc_map_root;
pvm_object_t *   long_gc_map_root_array;

/**
 * 
 * @brief Build map of objects.
 * 
 * Map is actually a hash tree with predefined
 * structure. There are three levels of tree
 * and list on a last, 4th level.
 * 
 * Tree level is made of full pages (4K), containing
 * pointers to lower level pages, just like in CPU
 * pagedir.
 * 
**/

static void long_gc_build_map( void )
{
    // Decide on size, must be power of 2

    size_t map_size = 1;
    while( map_size < lgc_mem_size )
        map_size <<= 1;

    LOG_INFO_(1, "Map size %x", map_size );

    lgc_map_size = map_size;

    // Each value is memory range per map element on given level
    long_gc_map_level_range[0] = map_size / ELEM_PER_PAGE;
    long_gc_map_level_range[1] = long_gc_map_level_range[0] / ELEM_PER_PAGE;
    long_gc_map_level_range[2] = long_gc_map_level_range[1] / ELEM_PER_PAGE;

    // Now create first two levels

    long_gc_map_root = pvm_create_array_sized( ELEM_PER_PAGE );
    long_gc_map_root_array = (void *)long_gc_map_root->da;

    int i;
    for(i = 0; i < ELEM_PER_PAGE; i++)
        long_gc_map_root_array = pvm_create_array_sized( ELEM_PER_PAGE );

}
/**
 * 
 * @brief Get pointer to final list object that will contain element.
 * 
 * Create intermediate tables if needed.
 * 
**/
static pvm_object_t long_gc_get_target_list( size_t address )
{
    // Shifted address - zero = start of our range
    size_t saddr = address - lgc_mem_start;

    size_t level0_elem = saddr / long_gc_map_level_range[0];
    size_t level0_more = saddr % long_gc_map_level_range[0];

    if( 0 == long_gc_map_root_array[level0_elem])
    {
        long_gc_map_root_array[level0_elem] = 
            pvm_create_array_sized( ELEM_PER_PAGE );
    }

    pvm_object_t l1object = long_gc_map_root_array[level0_elem];
    pvm_object_t *l1_array = (void *)l1object->da;

    size_t level1_elem = level0_more / long_gc_map_level_range[1];
    size_t level1_more = level0_more % long_gc_map_level_range[1];

    if( 0 == l1_array[level1_elem])
    {
        l1_array[level1_elem] = 
            pvm_create_array_sized( ELEM_PER_PAGE );
    }

    pvm_object_t l2object = l1_array[level1_elem];
    pvm_object_t *l2_array = (void *)l2object->da;

    size_t level2_elem = level1_more / long_gc_map_level_range[2];
    //size_t level2_more = level1_more % long_gc_map_level_range[2];

    if( 0 == l2_array[level2_elem])
    {
        l2_array[level2_elem] = 
            pvm_create_array_object(  );
    }

    // last one is a list - we use - ?

}

/**
 * @brief Put or update element.
 * 
 * If element is absent in map - add it.
 * 
 * Else - update status. Marked status can not be reset.
 * 
 * @param[in] address Address of object in memory
 * @param[in] visited We alredy marked all children of this object
 * @param[in] marked Object is reachable from roots.
 * @param[in] childfree Object has no chidlren.
 * 
**/
static void long_gc_map_put(
    size_t address, 
    int visited, int marked, int childfree)
{



}



static void long_gc_map_foreach()
{
    
}


static void long_gc_roots_phase(void)
{
    // Arenas
    // Root object
    // Be conservative? Get some pointers from kernel?
}

static void long_gc_scan_phase(void)
{

}

static void long_gc_mark_phase(void)
{

}




static void long_gc_sweep_phase(void)
{
    
}