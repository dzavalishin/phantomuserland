#ifdef ARCH_ia32
#if PAGING_PARTITION
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * IDE IO
 *
 *
**/

#define DEBUG_MSG_PREFIX "ide"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 12
#define debug_level_info 1

#include <errno.h>
#include <disk.h>
#include <disk_q.h>

#include <threads.h>
#include <kernel/config.h>
#include <kernel/stats.h>
#include <kernel/page.h>
#include <kernel/libkern.h>
#include <kernel/info/idisk.h>

#include <pager_io_req.h>

#include <hal.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <kernel/bus/pci.h>

#include "ataio.h"
#include <kernel/dpc.h>

#include <dev/ata.h>


static ataio_t         ata_buf; // temp global
ataio_t         *ata = &ata_buf;















// TODO pager does not check for io errors!

#define USE_LBA48 0
#define IO_PANIC 0
#define BLOCKED_IO 1
#define IDE_TRIM 0


static errno_t simple_ide_idenify_device(int dev);
static errno_t ide_reset( int ndev );



static hal_mutex_t      ide_io;
//static long 		disk_sectors[4];

static i_disk_t         disk_info[4];
static int              have_dev[4];


// TODO RENAME here and in phantom/dev/ata*
unsigned char buffer[PAGE_SIZE];

static char * devTypeStr[] = { "NO DEVICE", "UNKNOWN TYPE", "ATA", "ATAPI" };


void setup_simple_ide()
{
    hal_mutex_init( &ide_io, "IDE IO" );

    ata->int_use_intr_flag = 0;    // use INT mode if != 0
    ata->pio_base_addr1 = 0x1f0;
    ata->pio_base_addr2 = 0x3f0;

    ata->pio_bmide_base_addr = 0;
    ata->pio_memory_access = 0; // do we access IDE through memory

/*
    ata->pio_memory_dt_opt = PIO_MEMORY_DT_OPT0;


#if HAVE_WIDE_XFERS
    ata->pio_xfer_width = 16;
#else
    ata->pio_xfer_width = 8;
#endif
*/

    int bm_base_reg = 0xC000;

    // tell ATADRVR how big the buffer is
    ata->reg_buffer_size = PAGE_SIZE; //BUFFER_SIZE;

    SHOW_FLOW0( 1, "Init");

    pci_cfg_t cfg;
    int found = !phantom_pci_find( &cfg, 0x8086, 0x7010 );
    if(!found) found = !phantom_pci_find( &cfg, 0x1106, 0x0571 );

    if(!found)
    {
        SHOW_ERROR0( 0 , "PCI IDE - no known vendor/dev, looking up by class" );
        found = !phantom_pci_find_class( &cfg, 1, 1 ); // class 1.1 is IDE
    }

    if(found)
    {
        bm_base_reg = cfg.base[4];
        SHOW_INFO( 0 , "Found PCI IDE, BM base reg=0x%X, irq %d", bm_base_reg, cfg.interrupt );

        SHOW_INFO( 8 , "base 0 = 0x%X, base 1 = 0x%X, base 2 = 0x%X, base 3 = 0x%X, base 4 = 0x%X, base 5 = 0x%X", cfg.base[0], cfg.base[1], cfg.base[2], cfg.base[3], cfg.base[4], cfg.base[5] );
        SHOW_INFO( 8 , "size 0 = 0x%X, size 1 = 0x%X, size 2 = 0x%X, size 3 = 0x%X, size 4 = 0x%X, size 5 = 0x%X", cfg.size[0], cfg.size[1], cfg.size[2], cfg.size[3], cfg.size[4], cfg.size[5] );
    }
    else
        SHOW_ERROR0( 0 , "PCI IDE not found" );

    pio_set_iobase_addr( 0x1f0, 0x3f0, 0 );


    if(found)
    {
        //int bm_b2 = 0xc000+2;
        int bm_b2 = bm_base_reg+2;
        int rc;
        if( (rc = int_enable_irq( 0, 14, bm_b2, 0x1F0+7 )) )
            printf("Error %d enabling IDE irq\n", rc );

        int dma_rc = dma_pci_config( bm_base_reg );
        if(dma_rc)
            SHOW_ERROR( 0, "Error %d enabling IDE DMA", rc );
    }

    // 2) find out what devices are present -- this is the step
    // many driver writers ignore.  You really can't just do
    // resets and commands without first knowing what is out there.
    // Even if you don't care the driver does care.
    int numDev = reg_config();
    SHOW_INFO( 0, "Found %d devices, dev 0 is %s, dev 1 is %s.",
               numDev,
               devTypeStr[ ata->reg_config_info[0] ],
               devTypeStr[ ata->reg_config_info[1] ] );

    //ide_reset( 0 );
    //ide_reset( 1 );
}


size_t limit_nsect( int ndev, size_t nsect )
{
    if( nsect > disk_info[ndev].maxMultSectors )
        nsect = disk_info[ndev].maxMultSectors;

    // For some reason fails on QEMU if > 8
    if( nsect > 8 ) nsect = 8;
    if( nsect > 1 ) nsect &= ~1; // even num of sect

    return nsect;
}

static errno_t ide_reset( int ndev )
{
    hal_mutex_lock( &ide_io );
    int rc = reg_reset( 0, ndev );
    hal_mutex_unlock( &ide_io );

    if ( rc ) SHOW_ERROR( 0, "can't reset ide dev %d", ndev );
    return rc ? EIO : 0;
}


static errno_t ide_flush( int ndev )
{
    hal_mutex_lock( &ide_io );
    int rc = reg_non_data_lba28( ndev, CMD_FLUSH_CACHE, 0, 0, 0 );
    hal_mutex_unlock( &ide_io );

    return rc ? EIO : 0;
}

#if IDE_TRIM
static errno_t ide_trim( int ndev, long secno, size_t nsect )
{
    if( ndev > 3 ) return ENXIO;

    if( !(disk_info[ndev].has & I_DISK_HAS_TRIM) )
        return ENXIO;

    hal_mutex_lock( &ide_io );
    while( nsect > 0 )
    {
        size_t count = limit_nsect( ndev, nsect );

        SHOW_FLOW( 12, "start trim sect %d + %d", secno, count );
        int rc = reg_non_data_lba28( ndev, ATA_CMD_DSM,
                                     DSM_TRIM, count, // feature reg, sect count
                                     secno );
        SHOW_FLOW( 12, "end   trim sect %d", secno );

        if ( rc )
        {
            hal_mutex_unlock( &ide_io );
            return EIO;
        }
        secno += count;
        nsect -= count;
    }

    hal_mutex_unlock( &ide_io );
    return 0;
}
#endif


static errno_t ide_write( int ndev, long physaddr, long secno, size_t nsect )
{
    hal_mutex_lock( &ide_io );

#if BLOCKED_IO
    int tries = 5;
    //if(nsec > 1) printf("nsec %d\n", nsec );

retry:;
    while( nsect > 0 )
    {
        size_t count = limit_nsect( ndev, nsect );

        SHOW_FLOW( 12, "start sect wr sect %d + %d", secno, count );
#if 1
        int rc = dma_pci_lba28(
                               ndev, CMD_WRITE_DMA,
                               0, count, // feature reg, sect count
                               secno, physaddr, count );
#else
        // unsure, test
        int rc = reg_pio_data_in_lba28( ndev, int cmd,
                                  unsigned int fr, unsigned int sc,
                                  unsigned long lba,
                                  //unsigned int seg, unsigned int off,
                                  void *addr,
                                  long numSect, int multiCnt );
#endif
        SHOW_FLOW( 12, "end   sect wr sect %d", secno );

        if ( rc )
        {
            if( tries-- <= 0 )
            {
                hal_mutex_unlock( &ide_io );
                return EIO;
            }
            else
            {
                printf("IDE write failure sec %ld, retry\n", secno );
                goto retry;
            }
        }
        secno += count;
        nsect -= count;
        physaddr += (512*count);
    }


#else // BLOCKED_IO

    int i;
    for( i = nsect; i > 0; i-- )
    {
        int rc = dma_pci_lba28(
                               ndev, CMD_WRITE_DMA,
                               0, 1, // feature reg, sect count
                               secno, physaddr,
                               1L );
        if ( rc )
        {
            return EIO;
        }
        secno++;
        physaddr += 512;

    }
#endif

    hal_mutex_unlock( &ide_io );
    return 0;
}

static errno_t ide_read( int ndev, long physaddr, long secno, size_t nsect )
{
    int tries = 5;

    hal_mutex_lock( &ide_io );

#if BLOCKED_IO
    //if(nsec > 1) printf("nsec %d\n", nsect );

retry:;
    while( nsect > 0 )
    {
        size_t count = limit_nsect( ndev, nsect );

        SHOW_FLOW( 11, "start sect rd sect %d + %d", secno, count );
#if 1
#if USE_LBA48
        int rc = dma_pci_lba48(
                               ndev, CMD_READ_DMA,
                               0, count, // feature reg, sect count
                               0, secno, // hi lo sec no
                               physaddr, count );
#else
        int rc = dma_pci_lba28(
                               ndev, CMD_READ_DMA,
                               0, count, // feature reg, sect count
                               secno, physaddr, count );
#endif
#else
        // unsure, test
        int rc = reg_pio_data_in_lba28( ndev, CMD_READ_SECTORS,
                                  0, count,
                                  secno,
                                  physaddr,
                                  count, 1 );
#endif
        SHOW_FLOW( 12, "end   sect rd sect %d", secno );

        if ( rc )
        {
            if( tries-- <= 0 )
            {
                hal_mutex_unlock( &ide_io );
                return EIO;
            }
            else
            {
                printf("IDE read failure sec %ld, retry\n", secno );
                goto retry;
            }
        }
        secno += count;
        nsect -= count;
        physaddr += (512*count);
    }


#else // BLOCKED_IO

retry:;
    int i;
    for( i = nsect; i > 0; i-- )
    {
        SHOW_FLOW( 12, "start sect rd sect %d", secno );
        int rc = dma_pci_lba28(
                               ndev, CMD_READ_DMA,
                               0, 1, // feature reg, sect count
                               secno, physaddr,
                               1L );
        SHOW_FLOW( 12, "end   sect rd sect %d", secno );

        if ( rc )
        {
            if( tries-- <= 0 )
            {
#if IO_PANIC
                panic("IDE read failure sec %ld", secno );
#else
                hal_mutex_unlock( &ide_io );
                return EIO;
#endif
            }
            else
            {
                printf("IDE read failure sec %ld, retry\n", secno );
                goto retry;
            }
        }
        secno++;
        physaddr += 512;
    }

#endif // BLOCKED_IO

    hal_mutex_unlock( &ide_io );
    return 0;
}






// Serves both uits
static phantom_disk_partition_t *both;
static struct disk_q *ideq;
static dpc_request ide_dpc;


static void startIo( struct disk_q *q )
{
    ideq = q; // it is allways the same
    dpc_request_trigger( &ide_dpc, 0 );
}

static void dpc_func(void *a)
{
    (void) a;

    pager_io_request *rq = ideq->current;
    assert(rq);

    if( (rq->unit > 1) || (rq->unit < 0) )
    {
        SHOW_ERROR( 1, "wrong IDE unit %d", rq->unit );
        ideq->ioDone( ideq, ENXIO );
        return;
    }

    if( (rq->blockNo >= (int)disk_info[rq->unit].nSectors) || (rq->blockNo < 0) )
    {
        SHOW_ERROR( 1, "wrong IDE blk %d on unit %d", rq->blockNo, rq->unit );
        ideq->ioDone( ideq, EINVAL );
        return;
    }


    SHOW_FLOW( 7, "starting io on rq %p blk %d", rq, rq->blockNo );

    errno_t rc;

    if(rq->flag_pageout)
    {
        rc = ide_write( rq->unit, rq->phys_page, rq->blockNo, rq->nSect );
    }
    else
    {
        rc = ide_read( rq->unit, rq->phys_page, rq->blockNo, rq->nSect );
    }

    SHOW_FLOW( 7, "calling iodone on q %p, rq %p", ideq, rq );
    assert(rq == ideq->current);
    ideq->ioDone( ideq, rc ? EIO : 0 );
}

errno_t u0AsyncIo( struct phantom_disk_partition *p, pager_io_request *rq )
{
    rq->unit = 0;

    SHOW_FLOW( 11, "part io sect %d, shift %d, o sect %d", rq->blockNo, p->shift, rq->blockNo + p->shift );

    return both->asyncIo( both, rq );
}

errno_t u1AsyncIo( struct phantom_disk_partition *p, pager_io_request *rq )
{
    rq->unit = 1;

    SHOW_FLOW( 11, "part io sect %d, shift %d, o sect %d", rq->blockNo, p->shift, rq->blockNo + p->shift );

    return both->asyncIo( both, rq );
}


static errno_t ideDequeue( struct phantom_disk_partition *p, pager_io_request *rq )
{
    (void) p;

    if( 0 == both->dequeue )
        return ENODEV;

    SHOW_FLOW( 11, "ide dequeue rq %p", rq );

    return both->dequeue( both, rq );
}

static errno_t ideRaise( struct phantom_disk_partition *p, pager_io_request *rq )
{
    (void) p;

    if( 0 == both->raise )
        return ENODEV;

    SHOW_FLOW( 11, "ide raise rq prio %p", rq );

    return both->raise( both, rq );
}

// TODO stub!
static errno_t ideFence( struct phantom_disk_partition *p )
{
    (void) p;
    SHOW_ERROR0( 0, "Ide fence stub" );

    if( have_dev[0] ) ide_flush( 0 );
    if( have_dev[1] ) ide_flush( 1 );

    return 0;
}



static void make_unit_part( int unit, void *aio )
{
    errno_t rc = ide_reset( unit );
    if(rc)
    {
        SHOW_ERROR( 0, "skip IDE unit %d, can't reset", unit );
        return;
    }

    phantom_disk_partition_t *p = phantom_create_partition_struct( both, 0, disk_info[unit].nSectors );
    assert(p);
    p->flags |= PART_FLAG_IS_WHOLE_DISK;
    p->specific = (void *)-1; // disk supposed to have that
    p->base = 0; // and don't have this
    p->asyncIo = aio;
    p->dequeue = ideDequeue;
    p->raise = ideRaise;
    p->fence = ideFence;
#if IDE_TRIM
    p->trim = ideTrim;
#endif
    snprintf( p->name, sizeof(p->name), "Ide%d", unit );
    errno_t err = phantom_register_disk_drive(p);
    if(err)
        SHOW_ERROR( 0, "Ide %d err %d", unit, err );

}


void connect_ide_io(void)
{
    setup_simple_ide();

    dpc_request_init( &ide_dpc, dpc_func );

    have_dev[0] = !simple_ide_idenify_device(0);
    have_dev[1] = !simple_ide_idenify_device(1);

    long both_size = lmax(disk_info[0].nSectors, disk_info[1].nSectors);

    both = phantom_create_disk_partition_struct( both_size, 0, 0, startIo );

    if(have_dev[0])         make_unit_part( 0, u0AsyncIo );
    if(have_dev[1])         make_unit_part( 1, u1AsyncIo );
}



void phantom_check_disk_check_virtmem( void * a, int n )
{
    (void) a;
    (void) n;
}

void phantom_check_disk_save_virtmem( void * a, int n )
{
    (void) a;
    (void) n;
}




// TODO ata/atapi/cfata
errno_t simple_ide_idenify_device(int dev)
{
    assert(dev < 4);

    SHOW_INFO( 11, "--- Identifying IDE %d: ", dev );

    u_int16_t buf[256];
    int rc = reg_pio_data_in_lba28( dev, CMD_IDENTIFY_DEVICE, //CMD_IDENTIFY_DEVICE_PACKET,
                                   0, 0, 0L, buf, 1L, 0 );
    if(rc)
    {
        SHOW_ERROR( 0 , "can't identify IDE %d", dev );
        return EIO;
    }

    parse_i_disk_ata( &disk_info[dev], buf );
    dump_i_disk( &disk_info[dev] );

    return 0;
}



#endif // PAGING_PARTITION
#endif // ARCH_ia32
