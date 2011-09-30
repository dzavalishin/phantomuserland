/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Phantom 'filesystem' check and fix code. Incomplete.
 *
**/

#define DEBUG_MSG_PREFIX "fsck"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/amap.h>

#include <phantom_disk.h>
#include <errno.h>

#include "pager.h"

#define MAP_FREE 0
#define MAP_USED 1
#define MAP_UNKNOWN 2

static amap_t map; 
static int map_created = 0;


/* return nonzero if page is not in disk's size range */
static int out_of_disk(disk_page_no_t disk_block_num)
{
    // TODO must be error
    if(disk_block_num == 0)
        return 0; // blk num 0 is used as 'nothing'

    if(((unsigned long)disk_block_num) < ((unsigned long)pager_superblock_ptr()->disk_start_page))
        printf("FSCK: Warning: block number below disk start: %ld < %ld\n",
                (unsigned long)disk_block_num, (unsigned long)pager_superblock_ptr()->disk_start_page );

    // unsigned comparison will treat negatives as very big positives -> out of range
    return ((unsigned long)disk_block_num) > ((unsigned long)pager_superblock_ptr()->disk_page_count);
}


/**
 *
 * Create and clear use map.
 *
**/

//static void fsck_create_map(int start_state)
static void fsck_create_map()
{
    if(map_created)
        panic("FSCK: create map twice");

    // TODO fsck disk size HARDCODED - 256Gb
    //amap_init( &map, 0, 0x4000000, MAP_UNKNOWN );
    amap_init( &map, 0, ~0, MAP_UNKNOWN );
    map_created = 1;
}

/**
 *
 * Delete use map.
 *
**/


static void fsck_delete_map()
{
    amap_destroy( &map );
    map_created = 0;
}





static void visit_all( amap_elem_addr_t from, amap_elem_size_t n_elem, u_int32_t flags, void *arg )
{
    void (*i_func)(disk_page_no_t, int flags ) = arg;

    unsigned i;

    for( i = from; i < from+n_elem; i++)
    {
        i_func( (disk_page_no_t)i, flags );
    }

    //return 0;
}

static void iterate_map(void (*i_func)(disk_page_no_t disk_block_num, int flags), int flags)
{
    //amm_iterate_gen( &amm, amm_iterate_func, i_func, 0, ~0, flags, ~0 );
    amap_iterate_flags( &map, visit_all, i_func, flags );
}

#pragma GCC diagnostic ignored "-Wunused-function"
static void iterate_all(void (*i_func)(disk_page_no_t disk_block_num, int flags))
//__attribute__ ((unused))
{
    //amm_iterate_gen( &amm, amm_iterate_func, i_func, 0, ~0, 0, 0 );
    amap_iterate_all( &map, visit_all, i_func );
}





/**
 *
 * Mark block as used. If it is already marked as used. return 1.
 * Else return 0.
 *
**/

#pragma GCC diagnostic ignored "-Wunused-function"
static int fsck_mark_as_used( disk_page_no_t disk_block_num )
//__attribute__ ((unused))
{
    int modified;
    errno_t ret = amap_check_modify( &map, disk_block_num, 1, MAP_USED, &modified );
    if( ret ) panic("fsck amap error %d", ret);
    return !modified;
}


static int fsck_just_mark_as_used( disk_page_no_t disk_block_num )
{
    //fsck_mark_as_used( disk_block_num );
    amap_modify( &map, disk_block_num, 1, MAP_USED );
    return 0;
}

/**
 *
 * Mark block as free. If it is already marked as free. return 1.
 * Else return 0.
 *
**/

static int fsck_mark_as_free( disk_page_no_t disk_block_num )
//__attribute__ ((unused))
{
    int modified;
    errno_t ret = amap_check_modify( &map, disk_block_num, 1, MAP_FREE, &modified );
    if( ret ) panic("fsck amap error %d", ret);
    return !modified;
}

static int fsck_just_mark_as_free( disk_page_no_t disk_block_num )
{
    amap_modify(&map, disk_block_num, 1, MAP_FREE );
    return 0;
}


/**
 *
 * Mark all map blocks as free.
 *
**/


static void fsck_set_map_free()
{
    amap_modify(&map, 0, ~0, MAP_FREE );
}



/**
 *
 * Mark all map blocks as used.
 *
**/


static void fsck_set_map_used()
//__attribute__ ((unused))
{
    amap_modify(&map, 0, ~0, MAP_USED );
}



/**
 *
 * Do something with all the pages of the list, including list's
 * own pages. ie - mark free or used. Returns 1 if list is insane,
 * 0 else. If process_f function is zero, it is not called (use for
 * sanity check).
 *
**/


static int fsck_forlist( disk_page_no_t list_start, unsigned int check_magic, int (*process_f) (disk_page_no_t ) )
{
    int insane = 0;

    disk_page_cache         		curr_p;
    struct phantom_disk_blocklist* 	curr;

    disk_page_cache_init(&curr_p);

    disk_page_no_t next = list_start;
    while( next ) {
        if(out_of_disk(next))
        {
            SHOW_ERROR( 0, "FSCK: list structure block is out of disk, blk %d", next );
            insane = 1;
            break;
        }

        if(process_f) process_f( next );

        disk_page_cache_seek_sync( &curr_p, next );
        curr = (struct phantom_disk_blocklist *)disk_page_cache_data(&curr_p);

        disk_page_no_t currp_no = next; // for diag only
        next = curr->head.next;

        if( (check_magic != 0) && (curr->head.magic != check_magic) )
        {
            SHOW_ERROR( 0, "FSCK: list magic is wrong, 0x%x instead of 0x%x, blk %d", curr->head.magic, check_magic, currp_no );
            insane = 1;
            break;
        }

        unsigned int used = curr->head.used; // how many slots are used in this page

        if( used > N_REF_PER_BLOCK )
        {
            SHOW_ERROR( 0, "FSCK: overused list page, magic 0x%x, blk %d", curr->head.magic, currp_no );
            insane = 1;
            break;
        }

        if( used != N_REF_PER_BLOCK && next != 0 )
            SHOW_ERROR( 0, "FSCK warning: incomplete list page, magic 0x%x, blk %d", curr->head.magic, currp_no );

        if(process_f != NULL )
        {
            unsigned int i;
            for( i = 0; i < used; i++ )
            {
                disk_page_no_t lbn = curr->list[i];
                if (lbn)
                {
                    if(out_of_disk(lbn))
                    {
                        SHOW_ERROR( 0, "FSCK: list data block is out of disk, blk %d", lbn );
                        insane = 1;
                        break;
                    }
                    process_f(lbn);
                }
            }
        }

    }

    disk_page_cache_finish(&curr_p);

    return insane;
}



// Add all list blocks (including list itself) to map as used
static void fsck_list_justadd_as_used( disk_page_no_t list_start )
{
    fsck_forlist( list_start, 0, fsck_just_mark_as_used );
}

// Add all list blocks (including list itself) to map as free
static void fsck_list_justadd_as_free( disk_page_no_t list_start )
{
    fsck_forlist( list_start, 0, fsck_just_mark_as_free );
}





// Now check cross-use of the same blocks
static void fsck_list_crossuse( disk_page_no_t list_start )
//__attribute__ ((unused))
{
    (void) list_start;
    //fsck_forlist( list_start, fsck_just_mark_as_used );
    // no - do myself, have a copy of forlist here
    panic("unimplemented fsck_list_crossuse");
}
















static int phantom_fsck_super()
{
    return 0;
}
















static int phantom_fsck_do_list(disk_page_no_t *pnp, char *desc, int magic, int *corr)
{
    printf("-- %s... ", desc);
    if( *pnp == 0)
    {
        printf("None\n");
        return 0;
    }

    if( fsck_forlist( *pnp, magic, NULL ) )
    {
        *pnp = 0;
        *corr = 1;
        pager_update_superblock();
        printf("!! damaged, deleting !!\n");
        return 1;
    }

    printf("Ok\n");
    return 0;
}



/* return nonzero if something is wrong here */

static int phantom_fsck_lists()
{
    int corruption = 0;

    phantom_disk_superblock *sb = pager_superblock_ptr();

    // A1. check list magicks, delete lists that seem to be corrupt
    // A2. check lists sanity - pages have sane 'used' numbers,
    // all intermediate pages are full

    printf("-- Last snap... ");
    if( sb->last_snap && fsck_forlist( sb->last_snap, DISK_STRUCT_MAGIC_SNAP_LIST, NULL ) )
    {
        sb->last_snap = 0;
        sb->last_snap_time = 0;
        sb->last_snap_crc32 = 0;
        corruption = 1;
        pager_update_superblock();
    }
    else
        printf("Ok\n");


    printf("-- Prev snap... ");
    if( sb->prev_snap && fsck_forlist( sb->prev_snap, DISK_STRUCT_MAGIC_SNAP_LIST, NULL ) )
    {
        sb->prev_snap = 0;
        sb->prev_snap_time = 0;
        sb->prev_snap_crc32 = 0;
        corruption = 1;
        pager_update_superblock();
    }
    else
        printf("Ok\n");


    printf("-- Boot list... ");
    if( sb->boot_list && fsck_forlist( sb->boot_list, DISK_STRUCT_MAGIC_BOOT_LOADER, NULL ) )
    {
        sb->boot_list = 0;
        corruption = 1;
        pager_update_superblock();
    }
    else
        printf("Ok\n");

    /*
    if( sb->kernel_list && fsck_forlist( sb->kernel_list, DISK_STRUCT_MAGIC_BOOT_KERNEL, NULL ) )
    {
        sb->kernel_list = 0;
        corruption = 1;
        pager_update_superblock();
    }
    else
        printf("Ok\n");
        */
    phantom_fsck_do_list(&(sb->kernel_list), "Kernel", DISK_STRUCT_MAGIC_BOOT_KERNEL, &corruption );

    int i;
    for( i = 0; i < DISK_STRUCT_N_MODULES; i++ )
    {
        /*if( sb->boot_module[i] && fsck_forlist( sb->boot_module[i], DISK_STRUCT_MAGIC_BOOT_MODULE, NULL ) )
        {
            sb->boot_module[i] = 0;
            corruption = 1;
            pager_update_superblock();
            }*/
        if( (sb->boot_module[i]) == 0 )
            continue;

        char str[50];
        snprintf( str, 49, "Boot module %d", i );

        phantom_fsck_do_list(&(sb->boot_module[i]), str, DISK_STRUCT_MAGIC_BOOT_MODULE, &corruption );
    }

    // TODO - I don't know how to process bad list!
    // TDOO - bad list must have non-bad blocks in list structure

    /*if( sb->log_list && fsck_forlist( sb->log_list, DISK_STRUCT_MAGIC_LOG_LIST, NULL ) )
    {
        sb->log_list = 0;
        corruption = 1;
        pager_update_superblock();
    }*/

    phantom_fsck_do_list(&(sb->log_list), "Log list", DISK_STRUCT_MAGIC_LOG_LIST, &corruption );

    return corruption;
}



#pragma GCC diagnostic ignored "-Wunused-function"
static void phantom_fsck_rebuild_free()
//__attribute__ ((unused))
{
    //phantom_disk_superblock *sb = pager_superblock_ptr();

    // A. Create new usage map. Map shows everything as free.
    fsck_create_map();
    fsck_set_map_free();


    // B. Come through all the available lists, adding referred pages
    // to map.

    // B1. Snap 1
    // B2. Snap 2
    // B3. Kernel list
    // B4. Module lists
    // B5. etc... see superblock
    //fsck_forlist( snap1, fsck_list_justadd );

    // C. Now everything else is free - build free list again

    // D. Delete map.
    fsck_delete_map();
}















/*
 *
 * do_rebuild forces freelist rebuild even if corruption is not found.
 *
 */

void phantom_fsck(int do_rebuild )
{
    // PRE - disable snaps - or must be disabled before?

    // 0. TODO - replace disk block allocator with special one
    // that works with spare pool from p. 4 below. Of course,
    // we are not doing snaps during fsck anyway

    // 1. Check some superblock integrity

    SHOW_FLOW0( 0, "*** check superblock ***");
    if( phantom_fsck_super() ) do_rebuild = 1;

    // 2. check lists integrity, kill insane lists

    SHOW_FLOW0( 0, "*** check lists ***");
    if( phantom_fsck_lists() ) do_rebuild = 1;


    if(do_rebuild)
    {
        SHOW_ERROR0( 0, "*** errors found, rebuilding free list ***");
        // 3. Rebuild freelist

        //phantom_fsck_rebuild_free();

        // 4. Check disk blocks cross-use

        //phantom_fsck_check_crossuse();

        // 5. TODO - prepare a spare blocks pool to let system run during fsck
        // next time

        // 6. TODO - if cross-use was detected - rebuild freelist again?
    }

    // Q - what shell we do if cross-use detected?

    SHOW_FLOW0( 0, "*** Finish! ***");

}







// -----------------------------------------------------------------------
// Now this is what we do after any snapshot
// Free unused blocks from old snap
// -----------------------------------------------------------------------


static void free_snap_worker(disk_page_no_t toFree, int flags)
{
    // don't try to free blk 0 - zero is used as 'no block' marker
    if( toFree == 0 )
        return;

    if(flags != MAP_FREE )
    {
        printf("phantom_free_snap warning: nonfree block passed by iterator: %d\n", (int)toFree );
        return;
    }

    //SHOW_FLOW( 0, "Free old snap blk: %ld", (long)toFree );
    //printf( " %ld", (long)toFree );
    pager_free_page( toFree );
}

void phantom_free_snap(
                       disk_page_no_t old_snap_start,
                       disk_page_no_t curr_snap_start,
                       disk_page_no_t new_snap_start
                      )
{
    if( old_snap_start == 0 )
    {
        SHOW_FLOW0( 0, "*** No old snap, skip list deletion ***");
        return;
    }

    SHOW_FLOW0( 0, "*** freeing old snap ***");
    fsck_create_map();

    fsck_list_justadd_as_free( old_snap_start );
    fsck_list_justadd_as_used( curr_snap_start );
    fsck_list_justadd_as_used( new_snap_start );


    // go through list, free pages that are finally free in map
    iterate_map(free_snap_worker, MAP_FREE);
    pager_flush_free_list();

    // ERROR - list structure for old_snap_start has to be freed too

    fsck_delete_map();

}






