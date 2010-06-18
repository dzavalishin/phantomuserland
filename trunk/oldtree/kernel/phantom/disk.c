#include "disk.h"
#include "disk_part_pc.h"
#include <phantom_disk.h>
#include <assert.h>
#include <x86/phantom_page.h>
#include <malloc.h>
#include <phantom_libc.h>
#include <string.h>
#include <kernel/vm.h>

// ------------------------------------------------------------
// Basic partition processing proxies
// ------------------------------------------------------------

static errno_t checkRange( struct phantom_disk_partition *p, long blockNo, int nBlocks )
{
    if( blockNo < 0 || blockNo > p->size )
        return EINVAL;

    if( blockNo+nBlocks < 0 || blockNo+nBlocks > p->size )
        return EINVAL;

    return 0;
}

#if !IO_RQ_SLEEP
static errno_t partSyncRead( struct phantom_disk_partition *p, void *to, long blockNo, int nBlocks )
{
    assert(p->specific == 0);
    // Temp! Rewrite!
    assert(p->base->block_size == p->block_size);

    if( checkRange( p, blockNo, nBlocks ) )
        return EINVAL;

    return p->syncRead( p, to, blockNo+p->shift, nBlocks );
}


static errno_t partSyncWrite( struct phantom_disk_partition *p, const void *from, long blockNo, int nBlocks )
{
    assert(p->specific == 0);
    // Temp! Rewrite!
    assert(p->base->block_size == p->block_size);

    if( checkRange( p, blockNo, nBlocks ) )
        return EINVAL;

    return p->syncWrite( p, from, blockNo+p->shift, nBlocks );
}
#endif

// NB!! pager_io_request's disk block no is IGNORED! Parameter is used!
// Parameter has to be in partition's block size
static errno_t partAsyncIo( struct phantom_disk_partition *p, pager_io_request *rq )
{
    assert(p->specific == 0);
    // Temp! Rewrite!
    assert(p->base->block_size == p->block_size);

    if( checkRange( p, rq->blockNo, rq->nSect ) )
        return EINVAL;

    rq->blockNo += p->shift;

    p->asyncIo( p, rq );
    return 0;
}

/*
// NB!! pager_io_request's disk block no is IGNORED! Parameter is used!
// Parameter has to be in partition's block size
static errno_t partAsyncWrite( struct phantom_disk_partition *p, long blockNo, int nSect, pager_io_request *rq )
{
    assert(p->specific == 0);
    // Temp! Rewrite!
    assert(p->base->block_size == p->block_size);

    if( checkRange( p, blockNo, nSect ) )
        return EINVAL;

    rq->blockNo += p->shift;

    p->asyncWrite( p, rq );
    return 0;
} */

// ------------------------------------------------------------
// Upper level disk io functions
// ------------------------------------------------------------

#if IO_RQ_SLEEP

static errno_t startSync( phantom_disk_partition_t *p, void *to, long blockNo, int nBlocks, int isWrite )
{
    assert( p->block_size < PAGE_SIZE );
    int m = PAGE_SIZE/p->block_size;


    pager_io_request rq;

    pager_io_request_init( &rq );

    rq.phys_page = (physaddr_t)phystokv(to);
    rq.disk_page = blockNo;

    rq.blockNo = blockNo*m;
    rq.nSect   = nBlocks*m;

    if(isWrite) rq.flag_pageout = 1;
    else rq.flag_pagein = 1;

    rq.flag_sleep = 1; // Don't return until done

    return partAsyncIo( p, &rq );
}

errno_t phantom_sync_read_disk( phantom_disk_partition_t *p, void *to, long blockNo, int nBlocks )
{
    return startSync( p, to, blockNo, nBlocks, 0 );
}

errno_t phantom_sync_write_disk( phantom_disk_partition_t *p, const void *to, long blockNo, int nBlocks )
{
    return startSync( p, (void *)to, blockNo, nBlocks, 1 );
}


#else
errno_t phantom_sync_read_disk( phantom_disk_partition_t *p, void *to, long blockNo, int nBlocks )
{
    assert( p->block_size < PAGE_SIZE );
    int m = PAGE_SIZE/p->block_size;
    return p->syncRead( p, to, blockNo*m, nBlocks*m );
}

errno_t phantom_sync_write_disk( phantom_disk_partition_t *p, const void *to, long blockNo, int nBlocks )
{
    assert( p->block_size < PAGE_SIZE );
    int m = PAGE_SIZE/p->block_size;
    return p->syncWrite( p, to, blockNo*m, nBlocks*m );
}
#endif

void disk_enqueue( phantom_disk_partition_t *p, pager_io_request *rq )
{
    int m = PAGE_SIZE/p->block_size;
    rq->blockNo = rq->disk_page*m;
    rq->nSect = m;

    assert( rq->flag_ioerror == 0 );
    assert( rq->flag_pagein != rq->flag_pageout );

    p->asyncIo( p, rq );
}


// ------------------------------------------------------------
// Partitions list management
// ------------------------------------------------------------

static void find_subpartitions(phantom_disk_partition_t *p);
static void dump_partition(phantom_disk_partition_t *p);

static phantom_disk_partition_t partitions[MAX_DISK_PARTITIONS];
static int nPartitions = 0;

static void register_partition(phantom_disk_partition_t *p)
{
    if( nPartitions >= MAX_DISK_PARTITIONS )
    {
        printf("Too many partitions, skipping one (%.64s)\n", p->name );
        return;
    }

    partitions[nPartitions] = *p;
    dump_partition(p);
    find_subpartitions(p);
}


phantom_disk_partition_t *phantom_create_partition_struct(phantom_disk_partition_t *base, long shift, long size)
{
    phantom_disk_partition_t *ret = calloc( 1, sizeof(phantom_disk_partition_t) );

    ret->block_size = 512;
    ret->shift = shift;
    ret->size = size;
    ret->flags = 0;

    ret->specific = 0;

#if !IO_RQ_SLEEP
    ret->syncRead = partSyncRead;
    ret->syncWrite = partSyncWrite;
#endif

    ret->asyncIo = partAsyncIo;
    //ret->asyncRead = partAsyncRead;
    //ret->asyncWrite = partAsyncWrite;

    ret->base = base;

    return ret;
}

// ------------------------------------------------------------
// New disk registration entry point
// ------------------------------------------------------------

errno_t phantom_register_disk_drive(phantom_disk_partition_t *p)
{
    assert( p->specific != 0 );
    assert( p->shift == 0 );
    assert( p->size != 0 );
    assert( p->base == 0 );

    // Temp! Rewrite!
    assert( p->block_size == 512 );

    //p->flags |= PART_FLAG_IS_WHOLE_DISK;
    assert( p->flags & PART_FLAG_IS_WHOLE_DISK );

    register_partition( p );
    // todo check sanity - no partitions overlap on each level

    return 0;
}


// ------------------------------------------------------------
// Partitions parsing
// ------------------------------------------------------------

static void lookup_old_pc_partitions(phantom_disk_partition_t *p);
static void lookup_phantom_fs(phantom_disk_partition_t *p);

static void find_subpartitions(phantom_disk_partition_t *p)
{
    lookup_old_pc_partitions(p);
    lookup_phantom_fs(p);
}


static void lookup_old_pc_partitions(phantom_disk_partition_t *p)
{
    unsigned char buf[512];

    int lookAt =
        (p->flags & PART_FLAG_IS_WHOLE_DISK) ||
        (p->type == 0x05) ||
        (p->type == 0x0F) ||
        (p->type == 0x85);

    if(!lookAt ) return;

    //p->syncRead( p, buf, 0, 1 );
    if( phantom_sync_read_disk( p, buf, 0, 1 ))
        return;


    if( (buf[0x1FE] != 0x55) || (buf[0x1FF] != 0xAA) )
        return;

    p->flags |= PART_FLAG_IS_DIVIDED;

    int i; int pno = 0;
    for( i = 0x1BE; i <= 0x1EE; i += 16 )
    {
        struct pc_partition *pp = (struct pc_partition *)buf+i;

        if(pp->size == 0)
            continue; // break?

        phantom_disk_partition_t * newp = phantom_create_partition_struct( p, pp->start, pp->size);
        newp->type = pp->type;

        if(newp->type == PHANTOM_PARTITION_TYPE_ID)
        {
            printf("!! Phantom Partition found !!\n");
            p->flags |= PART_FLAG_IS_PHANTOM_TYPE;

        }

        char pn[4] = "PC0";
        pn[2] += pno++;
        strncpy(newp->name, pn, PARTITION_NAME_LEN-1);

        register_partition( newp );
    }

}



static disk_page_no_t sbpos[] = DISK_STRUCT_SB_OFFSET_LIST;
static int nsbpos = sizeof(sbpos)/sizeof(disk_page_no_t);

static void lookup_phantom_fs(phantom_disk_partition_t *p)
{
    char buf[PAGE_SIZE];
    phantom_disk_superblock *sb = (phantom_disk_superblock *)&buf;

    int i;
    for( i = 0; i < nsbpos; i++ )
    {
        if( phantom_sync_read_disk( p, buf, sbpos[i], 1 ) )
            continue;
        if( phantom_calc_sb_checksum( sb ) )
        {
            p->flags |= PART_FLAG_IS_PHANTOM_FSSB;
        }
    }
}



static void dump_partition(phantom_disk_partition_t *p)
{
    printf("Disk Partition %s (%s)\n", p->name, p->label );

    printf(" - type %d%s\n", p->type, p->type == PHANTOM_PARTITION_TYPE_ID ? " (phantom)" : "" );
    printf(" - flags %b\n", p->flags, "\020\1PhantomPartType\2PhantomFS\5Bootable\6Divided\7IsDisk" );
    printf(" - blksz %d, start %ld, size %ld\n", p->block_size, p->shift, p->size );
    printf(" - %s base, %s specific\n", p->base ? "has" : "no", p->specific ? "has" : "no" );

}

