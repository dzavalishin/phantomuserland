#if 0
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * ATA driver.
 * This one is not used yet. It will be available through the
 * new, partition-aware disk IO subsystem.
 *
 *
**/

#define DEBUG_MSG_PREFIX "ATA"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10



#include <ia32/pio.h>
#include <ia32/pci.h>
#include <device.h>

#include <disk_q.h>
#include <disk.h>

#include <hal.h>
#include <threads.h>

#include "ataio.h"

//#define DEV_NAME "ATA "

static char * devTypeStr[] = { "NO DEVICE", "UNKNOWN TYPE", "ATA", "ATAPI" };

static void intr_wait( ataio_t *ata );
static void intr_awake( ataio_t *ata );


static int ata_read( struct phantom_device *dev, void *buf, int len);
static int ata_write(struct phantom_device *dev, const void *buf, int len);

static errno_t ata_idenify_device( ataio_t *ata, int dev, long *retSize);

static void init_ata_thread( ataio_t *ata );
static void ata_thread_do_io( ataio_t *ata );

static void startIo( struct disk_q *q );

// Hack. Returning dev from probe limits us to one dev per driver. Here we select, which one. :)
static int drive = 1;



phantom_device_t * driver_ata_probe( pci_cfg_t *pci, int stage )
{
    ataio_t *ata = NULL;

    SHOW_FLOW0( 0, "probe" );

    ata = calloc( 1, sizeof(*ata) );

    if (ata == NULL)
    {
        SHOW_ERROR0( 0, "out of mem");
        return 0;
    }

    //ata->pio_xfer_width = 16; // Don't even think about 8 bit IDE controllers
    ata->pio_xfer_width = 8; // We need corresponding xfer code to be implemented, though
    ata->intr_wait = intr_wait;
    ata->intr_awake = intr_awake;

    int bm_base_reg = pci->base[4];
    int io_port;

    int i;
    for (i = 0; i < 6; i++)
    {
        if (pci->base[i] > 0xffff)
        {
            //dev->iomem = pci->base[i];
            //dev->iomemsize = pci->size[i];
            SHOW_INFO( 0, "base 0x%x, size 0x%x", pci->base[i], pci->size[i]);
        } else if( pci->base[i] > 0) {
            io_port = pci->base[i];
            SHOW_INFO( 0, "io_port 0x%x", io_port);
        }
    }

    //printf( DEV_NAME "stop\n");
    //ata_stop(ata);
    //hal_sleep_msec(10);

    SHOW_FLOW0( 0, "init");
    /*if (ata_init(ata) < 0)
    {
        //if(DEBUG)
        printf( DEV_NAME "init failed\n");
        // TODO cleanup
        //rtl8139_delete(ata);
        return 0;
    }*/

    unsigned int base1 = io_port;
    unsigned int base2 = io_port + 0x200; // BUG

    pio_set_iobase_addr( ata, base1, base2, bm_base_reg );

    int rc;
    if( (rc = int_enable_irq( ata, 0, pci->interrupt, bm_base_reg+2, base1+7 )) )
    {
        SHOW_ERROR( 0, "Error %d enabling IDE irq", rc );
        goto err1;
    }

    if( (bm_base_reg == 0) || dma_pci_config( ata, bm_base_reg ) )
    {
        ata->driver_busmaster = 0;
        SHOW_INFO0( 0, "Bus master mode is not available" );
    }
    else
    {
        ata->driver_busmaster = 1;
        SHOW_INFO( 0, "Bus master base = %d", bm_base_reg );
    }


    int numDev = reg_config(ata);
    SHOW_INFO( 0, "Found %d devices, dev 0 is %s, dev 1 is %s.\n",
               numDev,
               devTypeStr[ ata->reg_config_info[0] ],
               devTypeStr[ ata->reg_config_info[1] ] );



    rc = reg_reset( ata, 0, drive );
    if ( rc )
    {
        //panic("can't reset ide dev %d", dev );
        SHOW_ERROR( 0, "can't reset dev %d", drive );
        goto err1;
    }

    long size0; // in 512b sectors
    long size1;

    ata_idenify_device(ata, 0, &size0 );
    ata_idenify_device(ata, 1, &size1 );

    init_ata_thread( ata );


    static int seq_number = 0;

    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = "ATA disk driver";
    dev->seq_number = seq_number++;
    dev->drv_private = ata;

    dev->iobase = io_port;
    dev->irq = pci->interrupt;
    dev->iomem = 0; //phys_base;
    dev->iomemsize = 0; //phys_size;


    dev->dops.read = ata_read;
    dev->dops.write = ata_write;
    dev->dops.get_address = 0;


    if( size0 )
    {
        ata->dp0 = phantom_create_disk_partition_struct( size0, ata, 0, startIo );
        //errno_t rc =
        phantom_register_disk_drive(ata->dp0);
    }

    if( size1 )
    {
        ata->dp1 = phantom_create_disk_partition_struct( size1, ata, 1, startIo );
        //errno_t rc =
        phantom_register_disk_drive(ata->dp1);
    }

    return dev;


//err2:
//    free(dev);
err1:
    free(ata);
    return 0;
}



static int ata_read( struct phantom_device *dev, void *buf, int len)
{
    return -1;
}

static int ata_write(struct phantom_device *dev, const void *buf, int len)
{
    return -1;
}




static void startIo( struct disk_q *q )
{
    ataio_t *ata = q->device;
    //int drive = q->unit;

    SHOW_FLOW0(8, "START IO");
    hal_mutex_lock( &ata->mutex );

    if( ata->io_is_in_progress ) panic("have io_is_in_progress in ata_start_io");
    ata->io_is_in_progress = 1;

    ata->devSelect = q->unit;

    ata->startSector = q->current->blockNo; // SECTOR number!
    ata->nSectors = q->current->nSect; // Count of sectors to xfer

    ata->mem = q->current->phys_page;

    hal_cond_broadcast(&ata->start_io_sema);
    hal_mutex_unlock( &ata->mutex );
}









static void intr_wait( ataio_t *ata )
{
    hal_sleep_msec( 1 ); // Yield - in fact, we shall sleep until interrupt wakes us here
}


static void intr_awake( ataio_t *ata )
{
    // Do nothing as we use sleep above, not cond
}




// ------------------------------------------------------------------------
// Driver worker thread
//
// It's not efective, but IDE driver code we started with is
// structured for being run from thread. :(
// ------------------------------------------------------------------------




static void ata_device_io_thread( void *arg )
{
    ataio_t *ata = (ataio_t *)arg;

    hal_set_thread_name("AtaIO");

    SHOW_FLOW(9, "ata_device_io_thread started, ata = %p\n", ata);

    hal_mutex_lock( &ata->mutex );
    while(1)
    {
        ata->thread_ready = 1;
        hal_cond_wait(&ata->start_io_sema, &ata->mutex);


        SHOW_FLOW0(9, "thread awaken");
        ata_thread_do_io(ata);
    }
    hal_mutex_unlock( &ata->mutex );
}



static void init_ata_thread( ataio_t *ata )
{

    if(hal_cond_init( &(ata->start_io_sema), "AtaIO" ))
        panic("Ata sema creation failed");

    if(hal_mutex_init( &(ata->mutex), "AtaIO" ))
        panic("Ata mutex creation failed");

    ata->io_is_in_progress = 0;

    //dpc_request_init(&me->io_done_dpc, paging_device_io_done );

    // This virt addr is needed when disk is not DMA and we need to xfer data with CPU
//#if REMAPPED_PAGING_IO
    if( hal_alloc_vaddress( &ata->io_vaddr, 1 ) )
        panic("Pager alloc va failed");

    SHOW_FLOW(8, "Device IO virtual address is %p", ata->io_vaddr );
//#endif

    // Success or panic
    ata->tid = hal_start_kernel_thread_arg(ata_device_io_thread, ata);

    SHOW_FLOW0(8, "dev io thread started" );

    while(!ata->thread_ready)
        hal_sleep_msec(10);
}





















static int ata_write_page( ataio_t *ata )
{
    int tries = 5;
retry:;

    long secno = ata->startSector; //page_no * SECT_PER_PAGE;
    int ndev = ata->devSelect;
    int i, rc;

    void *va = ata->io_vaddr;
    physaddr_t pa = ata->mem;

    // TODO transfer in one op!
    for( i = ata->nSectors; i > 0; i-- )
    {

        if( ata->driver_busmaster )
        {
            rc = dma_pci_lba28( ata,
                                ndev, CMD_WRITE_DMA,
                                0, 1, // feature reg, sect count
                                secno, pa,
                                1L );
        }
        else
        {
            rc = reg_pio_data_out_lba28( ata,
                                        ndev, CMD_WRITE_SECTORS,
                                        0, 1, // feature reg, sect count
                                        secno, va,
                                        1L, 0 );
        }

        if ( rc )
        {
            if( tries-- <= 0 )
                panic("IDE write failure sect %ld", secno );
            else
            {
                printf("IDE write failure sect %ld, retry", secno );
                goto retry;
            }
        }


        secno++;
        va += 512;
        pa += 512;
    }

    return 0;
}

static int ata_read_page( ataio_t *ata )
{
    int tries = 5;
retry:;
    //long secno = page_no * SECT_PER_PAGE;
    long secno = ata->startSector; //page_no * SECT_PER_PAGE;
    int ndev = ata->devSelect;

    void *va = ata->io_vaddr;
    physaddr_t pa = ata->mem;

    int i;
    for( i = ata->nSectors; i > 0; i-- )
    {
        int rc;
        if( ata->driver_busmaster )
        {
            rc = dma_pci_lba28( ata,
                                   ndev, CMD_READ_DMA,
                                   0, 1, // feature reg, sect count
                                   secno, pa,
                                   1L );
        }
        else
        {
            rc = reg_pio_data_in_lba28( ata,
                                           ndev, CMD_READ_SECTORS,
                                           0, 1, // feature reg, sect count
                                           secno,
                                           va,
                                           1L, 0
                                          );
        }

        if ( rc )
        {
            if( tries-- <= 0 )
                panic("IDE read failure sect %ld", secno );
            else
            {
                printf("IDE read failure sect %ld, retry", secno );
                goto retry;
            }
        }
        secno++;
        va += 512;
        pa += 512;

    }
    return 0;
}




static void ata_thread_do_io(ataio_t *ata)
{
    SHOW_FLOW0(7, "thread executes request\n");

    if( !ata->io_is_in_progress ) panic("no io_is_in_progress in paging_device worker");

    SHOW_FLOW(8, "PhysAddr 0x%X\n", ata->mem );

    int rc;

    if( !ata->driver_busmaster )
    {
        assert(ata->io_vaddr);
        hal_page_control( ata->mem, ata->io_vaddr, page_map, page_rw );
    }

    if(ata->is_write)
    {
        SHOW_FLOW0(7, "WRITE... ");
        rc = ata_write_page( ata  );
    }
    else
    {
        SHOW_FLOW(7, "READ from sector %ld... ", ata->startSector );
        rc = ata_read_page( ata );
    }

    if( !ata->driver_busmaster )
    {
        SHOW_FLOW0(9, "Unmap... ");
        hal_page_control( ata->mem, ata->io_vaddr, page_unmap, page_noaccess );
    }

    SHOW_FLOW0(8, "Trigger IO completion callback ");
    //dpc_request_trigger( &ata->io_done_dpc, me);
    ata->q->ioDone( ata->q, rc ? EIO : 0 );
    SHOW_FLOW0(8, " ...DONE");

}


















static void intcpy( char *to, const char *from, int nwords )
{
    while( nwords-- )
    {
        to[0] = from[1];
        to[1] = from[0];

        to += 2;
        from += 2;
    }
}


// Fields in the structure returned by the IDENTIFY DEVICE (ECh) and
// IDENTIFY PACKET DEVICE (A1h) commands (WORD offsets)
#define IDENTIFY_FIELD_VALIDITY        53
#define IDENTIFY_DMA_MODES             63
#define IDENTIFY_ADVANCED_PIO          64
#define IDENTIFY_48BIT_ADDRESSING      83
#define IDENTIFY_COMMAND_SET_SUPPORT   83
#define IDENTIFY_COMMAND_SET_ENABLED   86
#define IDENTIFY_UDMA_MODES            88


// retSize is in 4096 byte blocks

static errno_t ata_idenify_device( ataio_t *ata, int dev, long *retSize)
{
    *retSize = 0;
    SHOW_INFO( 0, "--- Identifying IDE %d: ", dev );

    u_int16_t buf[256];
    int rc = reg_pio_data_in_lba28( ata,
                                   dev, CMD_IDENTIFY_DEVICE, //CMD_IDENTIFY_DEVICE_PACKET,
                                   0, 0,
                                   0L, //0L,
                                   buf, 1L, 0 );
    if(rc)
    {
        SHOW_ERROR( 0 , "can't identify IDE %d", dev );
        return EIO;
    }

    //int isATA = buf[0] & 0x8000;
    int isATA = (buf[0] >> 15) & 0x1;


    int isRemovable = buf[0] & 0x0080;
    int is48bitAddr = (buf[IDENTIFY_48BIT_ADDRESSING] >> 10) & 0x1;

#if 1
    if( !isATA )
    {
        SHOW_ERROR0( 0, "Not an ATA device?");
        //return ENXIO;
    }
#endif

    u_int32_t size = (buf[61] << 16) | buf[60];
    //printf( "nSect=%d / %d ", buf[60], buf[61] );


    char serial[21];
    intcpy( serial, ((char*)&buf)+10*2, 10 );
    serial[20] = 0;

    char model[41];
    intcpy( model, ((char*)&buf)+27*2, 20 );
    model[40] = 0;

    SHOW_INFO( 0, " is %s ATA, nSect=%d Mb Model: '%s' %s, %s bit LBA",
               isATA ? "" : "not",
               size/2048, model,
               isRemovable ? "removable" : "fixed", is48bitAddr ? "48" : "28"
             );

    if( !(buf[49] & 0x0300) )
    {
        SHOW_ERROR0( 0, "Not LBA/DMA disk\n");
        return ENXIO;
    }

    if( isATA && size )
        *retSize = size;


    return 0;
}




#endif


