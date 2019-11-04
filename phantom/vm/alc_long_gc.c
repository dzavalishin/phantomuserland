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
#include <vm/internal_da.h>

#include <kernel/stats.h>
#include <kernel/atomic.h>

// TODO 64 bit problem - 512?
#define ELEM_PER_PAGE 1024
//#define ELEM_PER_PAGE (PAGE_SIZE/sizeof(void *))


//static void long_gc_build_map(size_t address_range);
static void long_gc_build_map( void );
static void long_gc_roots_phase(void);
static void long_gc_scan_phase(void);
static void long_gc_mark_phase(void);
static void long_gc_sweep_phase(void);

static errno_t long_snapshot_get_object_header( pvm_object_t out, size_t mem_addr );
static errno_t long_snapshot_get_object_data( void * out, size_t mem_addr, size_t data_size );

static void * long_snapshot_cache_read_buffer( size_t buf_size, size_t mem_addr );
static errno_t long_snapshot_cache_write_buffer( void* buffer, size_t buf_size, size_t mem_addr );
static void * long_snapshot_cache_alloc_buffer( size_t buf_size );

static errno_t long_snapshot_read_buffer( void* buffer, size_t buf_size, size_t mem_addr );

static void lock_snapshot_from_deletion( bool lock );


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

    size_t lgc_map_size = map_size;

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

    LOG_FLOW( 5, "l0 elem %d more %d", level0_elem, level0_more );

    if( 0 == long_gc_map_root_array[level0_elem])
    {
        long_gc_map_root_array[level0_elem] = 
            pvm_create_array_sized( ELEM_PER_PAGE );
    }

    pvm_object_t l1object = long_gc_map_root_array[level0_elem];
    pvm_object_t *l1_array = (void *)l1object->da;

    size_t level1_elem = level0_more / long_gc_map_level_range[1];
    size_t level1_more = level0_more % long_gc_map_level_range[1];

    LOG_FLOW( 5, "l1 elem %d more %d", level1_elem, level1_more );

    if( 0 == l1_array[level1_elem])
    {
        l1_array[level1_elem] = 
            pvm_create_array_sized( ELEM_PER_PAGE );
    }

    pvm_object_t l2object = l1_array[level1_elem];
    pvm_object_t *l2_array = (void *)l2object->da;

    size_t level2_elem = level1_more / long_gc_map_level_range[2];
    size_t level2_more = level1_more % long_gc_map_level_range[2];

    LOG_FLOW( 5, "l2 elem %d more %d", level2_elem, level2_more );

    if( 0 == l2_array[level2_elem])
    {
        l2_array[level2_elem] = 
            pvm_create_array_object(  ); // Right? Do we use array?
    }

    // last one is a list
    return l2_array[level2_elem];

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

/**
 * 
 * @brief Function to use as iterator callack
 * 
 * @param obj_addr Address of object we visit
 * @param obj_header Header of object we visit
 * @param arg Argument passed to iterator func.
 * 
**/
typedef int (*long_gc_foreach_t)( size_t obj_addr, pvm_object_t obj_header, void *arg );

/**
 * 
 * @brief Iterate through map objects calling func for each.
 * 
**/
static errno_t long_gc_map_foreach(long_gc_foreach_t func, void *arg, size_t start_addr, size_t size )
{
    int i,j,k,l;
    for( i = 0; i < ELEM_PER_PAGE; i++)
    {
        pvm_object_t l0 = long_gc_map_root_array[i];
        if( l0 == 0) continue;
        pvm_object_t *l1_array = (void *)l0->da;

        for( j = 0; j < ELEM_PER_PAGE; j++ )
        {
            pvm_object_t l1 = l1_array[j];
            if( l1 == 0) continue;
            pvm_object_t *l2_array = (void *)l1->da;

            for( k = 0; k < ELEM_PER_PAGE; k++ )
            {
                pvm_object_t l2 = l2_array[j];
                if( l2 == 0) continue;
                //pvm_object_t *list = (void *)l1object->da;

                // now scan through list

            }

        }
    }

}


/**
 * 
 * @brief Iterate through snapshot objects calling func for each.
 * 
**/
static errno_t long_gc_snap_foreach(long_gc_foreach_t func, void *arg)
{
    
}


// -----------------------------------------------------------------------
// Callbacks
// -----------------------------------------------------------------------

int long_gc_find_root( size_t obj_addr, pvm_object_t obj_header, void *arg )
{
    if( obj_header->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_ROOT )
    {
        long_gc_map_put( obj_addr, 0, 1, 0 );
        return EEXIST;
    }

    // TODO have some limit?
    // TODO look in static arena first?

    return 0;
}

// -----------------------------------------------------------------------
// MAIN CODE
// -----------------------------------------------------------------------


static void long_gc_roots_phase(void)
{
    errno_t rc;
    // Arenas
    size_t arena_addr = 0;
    while( arena_addr < lgc_mem_end)
    {
        pvm_object_storage_t arena_hdr;
        rc = long_snapshot_get_object_header( &arena_hdr, arena_addr );
        assert(rc == 0);

        // check that it is arena
        assert( arena_hdr._flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_ARENA);
        // TODO more checks

        // find out another arena addr
        struct data_area_4_arena ada;
        rc = long_snapshot_get_object_data( &ada, arena_addr + sizeof(pvm_object_storage_t), sizeof(ada) );
        assert( 0 == rc);

        long_gc_map_put( arena_addr, 0, 1, 0 );


        arena_addr += ada.size;

    }

    // Find and add root object
    rc = long_gc_snap_foreach( long_gc_find_root, 0 );
    assert( rc = EEXIST );

    // Be conservative? Get some pointers from kernel?
}

static void long_gc_scan_phase(void)
{
again: ;    
    // scan part of map processing all unvisited objects
    // limit scan to small range to attempt to be inside of
    // cache for snapshot data


    size_t part_start;
    size_t part_size = 1024*1024*16; // 16M in one run - TODO cache size must be no less!


}

static void long_gc_mark_phase(void)
{

}




static void long_gc_sweep_phase(void)
{
    
}

// -----------------------------------------------------------------------
// Cache/IO interface
// -----------------------------------------------------------------------

/**
 * 
 * @brief Read part of memory from the cache or from snapshot
 * 
 * 
 * @param[out] buffer Where to put read data
 * @param[in] buf_size Size of buffer, bytes
 * @param[in] mem_addr Address of memory for the beginning of the buffer. PAGE ALIGNED!
 * 
 * Current requirements for alignment is 4096 bytes. 
 * 
**/
static void *long_snapshot_get_buffer( size_t buf_size, size_t mem_addr )
{
    errno_t rc;

    void *buf = long_snapshot_cache_read_buffer( buf_size, mem_addr );
    if( buf != 0 ) return buf;

    buf = long_snapshot_cache_alloc_buffer( buf_size );

    rc =  long_snapshot_read_buffer( buf, buf_size, mem_addr );
    assert(rc == 0);

    long_snapshot_cache_write_buffer( buf, buf_size, mem_addr );
}

/**
 * 
 * @brief Read object header fron snapshot.
 * 
 * @param[out] out Object header storage to read to
 * @param[in] mem_addr Address of object to read
 * 
**/
static errno_t long_snapshot_get_object_header( pvm_object_t out, size_t mem_addr )
{
    size_t pg_mem_addr = PREV_PAGE_ALIGN(mem_addr);

    void* buf;
    buf = long_snapshot_get_buffer( PAGE_SIZE, pg_mem_addr );

    size_t end_of_obj_header = mem_addr + sizeof(struct pvm_object_storage); // TODO check size
    size_t obj_shift = mem_addr - pg_mem_addr;

    if( pg_mem_addr + PAGE_SIZE > end_of_obj_header )
    {
        memcpy( out, buf + obj_shift, sizeof(struct pvm_object_storage) );
        return 0;
    }

    size_t part1_size = PAGE_SIZE-obj_shift;
    size_t part2_size = sizeof(struct pvm_object_storage) - part1_size;

    memcpy( out, buf + obj_shift, part1_size );

    buf = long_snapshot_get_buffer( PAGE_SIZE, pg_mem_addr+PAGE_SIZE );
    memcpy( out+part1_size, buf, part2_size );

    return 0;
}

/**
 * 
 * Read any data from snapshot.
 * 
 * 
**/
static errno_t long_snapshot_get_object_data( void * out, size_t mem_addr, size_t data_size )
{
    size_t pg_mem_addr = PREV_PAGE_ALIGN(mem_addr);

    void* buf;
    buf = long_snapshot_get_buffer( PAGE_SIZE, pg_mem_addr );

    size_t end_of_data = mem_addr + sizeof(struct pvm_object_storage); // TODO check size
    size_t obj_shift = mem_addr - pg_mem_addr;

    if( pg_mem_addr + PAGE_SIZE > end_of_data )
    {
        memcpy( out, buf + obj_shift, data_size );
        return 0;
    }

    size_t part1_size = PAGE_SIZE-obj_shift;
    size_t part2_size = data_size - part1_size;

    memcpy( out, buf + obj_shift, part1_size );

    while( part2_size > 0)
    {
        pg_mem_addr += PAGE_SIZE;

        buf = long_snapshot_get_buffer( PAGE_SIZE, pg_mem_addr );
        memcpy( out+part1_size, buf, min( part2_size, PAGE_SIZE) );

        part1_size += PAGE_SIZE;
        part2_size -= PAGE_SIZE;
    }

    return 0;
}

// -----------------------------------------------------------------------
// Cache interface
// -----------------------------------------------------------------------

/**
 * 
 * @brief Read part of memory from the cache
 * 
 * 
 * @param[out] buffer Where to put read data
 * @param[in] buf_size Size of buffer, bytes
 * @param[in] mem_addr Address of memory for the beginning of the buffer. PAGE ALIGNED!
 * 
 * Current requirements for alignment is 4096 bytes. 
 * 
**/
static void * long_snapshot_cache_read_buffer( size_t buf_size, size_t mem_addr )
{
// TODO keep some map of pointers into list of snapshot buffers
// TODO restructure snapshot to be a tree 
    return 0;
}

/**
 * 
 * @brief Write part of memory to the cache
 * 
 * 
 * @param[in] buffer Data to put to cache
 * @param[in] buf_size Size of buffer, bytes
 * @param[in] mem_addr Address of memory for the beginning of the buffer. PAGE ALIGNED!
 * 
 * Current requirements for alignment is 4096 bytes. 
 * 
**/
static errno_t long_snapshot_cache_write_buffer( void* buffer, size_t buf_size, size_t mem_addr )
{
// TODO keep some map of pointers into list of snapshot buffers
// TODO restructure snapshot to be a tree 
    return ENXIO;
}

/**
 * 
 * Allocate cache buffer, maybe freeing some other one.
 * 
**/
static void * long_snapshot_cache_alloc_buffer( size_t buf_size )
{
    assert(buf_size == 4096);
    static char buf[4096];
    return buf;
}

// -----------------------------------------------------------------------
// Snapshot interface
// -----------------------------------------------------------------------

/**
 * 
 * @brief Tell the snapshot subsystem that we use snapshot
 * 
 * Or else snapshot would be removed.
 * 
**/
static void lock_snapshot_from_deletion( bool lock )
{

}

/**
 * 
 * @brief Read part of memory from the snapshot.
 * 
 * 
 * @param[out] buffer Where to put read data
 * @param[in] buf_size Size of buffer, bytes
 * @param[in] mem_addr Address of memory for the beginning of the buffer. PAGE ALIGNED!
 * 
 * Current requirements for alignment is 4096 bytes. 
 * 
**/
static errno_t long_snapshot_read_buffer( void* buffer, size_t buf_size, size_t mem_addr )
{
// TODO keep some map of pointers into list of snapshot buffers
// TODO restructure snapshot to be a tree 

}

