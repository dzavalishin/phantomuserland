#ifdef ARCH_ia32
#if !PAGING_PARTITION
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Paging IO
 *
 *
**/

#define TEST_NEW_DISK_IO_STACK 1




#define IDE_INTR 1
#define IDE_DMA 1

#define REMAPPED_PAGING_IO (!IDE_DMA)



#define DEBUG_MSG_PREFIX "disk"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <errno.h>
#include <threads.h>
#include <kernel/config.h>
#include <kernel/stats.h>
#include <kernel/page.h>

#include "paging_device.h"
#include <hal.h>

#include <assert.h>
#include <string.h>

#include <kernel/bus/pci.h>

#include "ataio.h"


#if TEST_NEW_DISK_IO_STACK
static hal_mutex_t      ide_io;
void connect_ide_io(void);
static dpc_request ide_dpc;
static void dpc_func(void *a);

#endif



static int disk_sectors[4]; // max 4 devs


static void        	paging_device_start_io(paging_device *me);
static void        	paging_device_io_done( void *arg ); // arg is 'paging_device *me'

static void 		test_ide_io(void);

errno_t simple_ide_idenify_device(int dev);


#if !defined(IDE_DMA) && defined(REMAPPED_PAGING_IO)
#error wont work this way
#endif



// a little function to display all the error
// and trace information from the driver

void ShowAll( void );


void ClearTrace( void );



char * devTypeStr[] = { "NO DEVICE", "UNKNOWN TYPE", "ATA", "ATAPI" };

unsigned char buffer[PAGE_SIZE];


void setup_simple_ide()
{
    int bm_base_reg = 0xC000;

    // tell ATADRVR how big the buffer is
    ata->reg_buffer_size = PAGE_SIZE; //BUFFER_SIZE;

    SHOW_FLOW0( 1, "Init");

    pci_cfg_t cfg;
    int found = !phantom_pci_find( &cfg, 0x8086, 0x7010 );
    //int found = !phantom_pci_find_class( &cfg, 1 ); // class 1 is IDE

    if(found)
    {
        bm_base_reg = cfg.base[4];
        SHOW_INFO( 0 , "Found PCI IDE, BM base reg=0x%X", bm_base_reg );
    }

    pio_set_iobase_addr( 0x1f0, 0x3f0, 0 );


#if IDE_INTR
    if(found) {
        int rc;
        if( (rc = int_enable_irq( 0, 14, 0xc000+2, 0x1F0+7 )) )
            printf("Error %d enabling IDE irq\n", rc );

#if 0 || IDE_DMA
        int dma_rc = dma_pci_config( bm_base_reg );
        if(dma_rc)
            printf("Error %d enabling IDE DMA\n", rc );
#endif
    }
    //unsigned int bmAddr, unsigned int ataAddr )
#endif
    // 2) find out what devices are present -- this is the step
    // many driver writers ignore.  You really can't just do
    // resets and commands without first knowing what is out there.
    // Even if you don't care the driver does care.
    int numDev = reg_config();
    SHOW_INFO( 0, "Found %d devices, dev 0 is %s, dev 1 is %s.\n",
               numDev,
               devTypeStr[ ata->reg_config_info[0] ],
               devTypeStr[ ata->reg_config_info[1] ] );

    int dev = 1;

    int rc = reg_reset( 0, dev );
    if ( rc )
        panic("can't reset ide dev %d", dev );


    test_ide_io();

    simple_ide_idenify_device(0);
    simple_ide_idenify_device(1);
//getchar();

#if TEST_NEW_DISK_IO_STACK
    hal_mutex_init( &ide_io, "IDE IO" );

    dpc_request_init( &ide_dpc, dpc_func);


    //connect_ide_io();
#endif

}




static const int simle_io_dev = 1;
static const int vmem_check_dev = 0;

static const int SECT_PER_PAGE = 4096/512;

static void simple_ide_write_page( void *buf, long physaddr, long page_no, int ndev )
{
    long secno = page_no * SECT_PER_PAGE;
//printf("ide write %d\n", page_no); //hexdumpb( 0, buf, 256 ); getchar();

#if TEST_NEW_DISK_IO_STACK
    hal_mutex_lock( &ide_io );
#endif

    int i;
    for( i = SECT_PER_PAGE; i > 0; i-- )
    {

#if IDE_DMA
        int rc = dma_pci_lba28(
               ndev, CMD_WRITE_DMA,
               0, 1, // feature reg, sect count
               secno, physaddr,
               1L );
#else
        int rc = reg_pio_data_out_lba28(
               ndev, CMD_WRITE_SECTORS,
               0, 1, // feature reg, sect count
               secno, buf,
               1L, 0 );
#endif
        if ( rc )
            panic("IDE write failure pg %ld", page_no);
        secno++;
        buf += 512;
        physaddr += 512;

    }

#if TEST_NEW_DISK_IO_STACK
    hal_mutex_unlock( &ide_io );
#endif
}

static void simple_ide_read_page( void *_buf, long physaddr, long page_no, int ndev )
{
    int tries = 5;

#if TEST_NEW_DISK_IO_STACK
    hal_mutex_lock( &ide_io );
#endif

retry:;
    void *buf = _buf;
    long secno = page_no * SECT_PER_PAGE;
//printf("ide read %d...", page_no);
    int i;
    for( i = SECT_PER_PAGE; i > 0; i-- )
    {




#if IDE_DMA
        int rc = dma_pci_lba28(
                               ndev, CMD_READ_DMA,
                               0, 1, // feature reg, sect count
                               secno, physaddr,
                               1L );
#else
        int rc = reg_pio_data_in_lba28(
                                       ndev, CMD_READ_SECTORS,
                                       0, 1, // feature reg, sect count
                                       secno,
                                       buf,
                                       1L, 0
                                      );
#endif

        if ( rc )
        {
            if( tries-- <= 0 )
                panic("IDE read failure pg %ld", page_no );
            else
            {
                printf("IDE read failure pg %ld, retry", page_no );
                goto retry;
            }
        }
        secno++;
        buf += 512;
        physaddr += 512;

    }

#if TEST_NEW_DISK_IO_STACK
    hal_mutex_unlock( &ide_io );
#endif

}





//---------------------------------------------------------------------------



void paging_device_thread_do_io(paging_device *me)
{
    SHOW_FLOW0(7, "paging_device_io_thread executes request\n");


    if( !me->io_is_in_progress ) panic("no io_is_in_progress in paging_device worker");


    SHOW_FLOW(8, "PhysAddr 0x%X\n", me->mem );

#if REMAPPED_PAGING_IO
    hal_page_control( me->mem, me->io_vaddr, page_map, page_rw );

    if(me->is_write)
    {
        SHOW_FLOW0(7, "paging_device_io_thread WRITE... ");
        simple_ide_write_page( me->io_vaddr, me->mem, me->disk, simle_io_dev );
    }
    else
    {
        SHOW_FLOW(7, "paging_device_io_thread READ from pg %d... ", me->disk );
        simple_ide_read_page( me->io_vaddr, me->mem, me->disk, simle_io_dev );
    }

    SHOW_FLOW0(9, "Unmap... ");
    hal_page_control( me->mem, me->io_vaddr, page_unmap, page_noaccess );
    SHOW_FLOW0(7, " ...DONE\n");


#else

    if(me->is_write)
    {
        SHOW_FLOW0(7, "paging_device_io_thread WRITE... ");
        simple_ide_write_page( 0, me->mem, me->disk, simle_io_dev );
    }
    else
    {
        SHOW_FLOW(7, "paging_device_io_thread READ from pg %d... ", me->disk );
        simple_ide_read_page( 0, me->mem, me->disk, simle_io_dev );
    }
    SHOW_FLOW0(7, " ...DONE\n");

    STAT_INC_CNT(STAT_CNT_BLOCK_IO);

#endif

    SHOW_FLOW0(8, "Trigger IO completion DPC... ");
    dpc_request_trigger( &me->io_done_dpc, me);
    SHOW_FLOW0(8, " ...DONE\n");

}


static volatile int paging_device_io_thread_ready = 0;

void
paging_device_io_thread( void *arg )
{
    paging_device *me = (paging_device *)arg;

    hal_set_thread_name("Pager Dev");
	hal_set_current_thread_priority(PHANTOM_SYS_THREAD_PRIO);

    SHOW_FLOW(9, "paging_device_io_thread started, me = %lx\n", me);

    hal_mutex_lock( &me->mutex );
    while(1)
    {
        paging_device_io_thread_ready = 1;
        hal_cond_wait(&me->start_io_sema, &me->mutex);


        SHOW_FLOW0(9, "PG dev thread awaken\n");
        paging_device_thread_do_io(me);
    }
    hal_mutex_unlock( &me->mutex );
}







void init_paging_device(paging_device *me, const char *devname, int n_pages )
{
    (void) devname;

    if(hal_cond_init( &me->start_io_sema, "PagerIO" ))
        panic("Paging sema creation failed");

    if(hal_mutex_init( &me->mutex, "PagerIO" ))
        panic("Paging mutex creation failed");

    me->io_is_in_progress = 0;

    dpc_request_init(&me->io_done_dpc, paging_device_io_done );

    setup_simple_ide();

    me->n_pages = n_pages;

#if REMAPPED_PAGING_IO
    if( hal_alloc_vaddress( &me->io_vaddr, 1 ) )
        panic("Pager alloc va failed");
    SHOW_FLOW(8, "Device IO virtual address is 0x%X\n", me->io_vaddr );
#endif

    // Success or panic
    me->tid = hal_start_kernel_thread_arg(paging_device_io_thread, me);

    SHOW_FLOW0(8, "after starting pg dev io thread\n" );

    while(!paging_device_io_thread_ready)
        hal_sleep_msec(10);
}

// Called in DPC context
static void
paging_device_io_done(void *arg)
{
    paging_device *me = (paging_device *)arg;
    if( !me->io_is_in_progress ) panic("no io_is_in_progress in file_paging_device::io_done");

    // Save a copy of func p for 'me->io_is_in_progress = 0'
    // enables for one more IO to be started and callback rewritten
    void                (*cbf)() = me->callback;

    me->io_is_in_progress = 0;

    if(cbf) cbf();
}



static void
paging_device_start_io(paging_device *me)
{
    hal_mutex_lock( &me->mutex );
    if( me->io_is_in_progress ) panic("have io_is_in_progress in paging_device_stop_io");
    SHOW_FLOW0(8, "paging device START IO ");
    me->io_is_in_progress = 1;
    hal_cond_broadcast(&me->start_io_sema);
    hal_mutex_unlock( &me->mutex );
}




void
paging_device_start_read(
                         paging_device *me,

                         disk_page_no_t disk,
                         physaddr_t mem,
                         void (*callback)() )
{
    if( me->io_is_in_progress ) panic("io_is_in_progress in start read");

    me->callback   = callback;
    me->disk   = disk;
    me->mem    = mem;
    me->is_write = 0;

    SHOW_FLOW(8, "paging_device_start_read PA 0x%x dsk %d \n", me->mem, me->disk );

    //if(disk < 0 || disk >= me->n_pages)
    if(disk >= (unsigned)me->n_pages)
        panic("disk page address is out of range: %d (%d max)", disk, me->n_pages );

    paging_device_start_io(me);
}


void
paging_device_start_write(
                           paging_device *me,

                           disk_page_no_t disk,
                           physaddr_t mem,
                           void (*callback)() )
{
    if( me->io_is_in_progress ) panic("io_is_in_progress in start write");

    me->callback   = callback;
    me->disk   = disk;
    me->mem    = mem;
    me->is_write = 1;

    paging_device_start_io(me);
}

void
paging_device_start_read_rq (paging_device *me, pager_io_request *req, void (*callback)() )
{
    //paging_device_start_read( me, req->disk_page, req->phys_page, req->virt_addr, callback );
    paging_device_start_read( me, req->disk_page, req->phys_page, callback );
}

void
paging_device_start_write_rq(paging_device *me, pager_io_request *req, void (*callback)() )
{
    //paging_device_start_write( me, req->disk_page, req->phys_page, req->virt_addr, callback );
    paging_device_start_write( me, req->disk_page, req->phys_page, callback );
}








//**********************************************


void ClearTrace( void )

{

   // clear the command history and low level traces
   trc_cht_dump0();     // zero the command history
   trc_llt_dump0();     // zero the low level trace
}




void ShowAll( void )

{
   int lc = 0;
   unsigned const char * cp;

   //printf( "ERROR !\n" );

   // display the command error information
   trc_err_dump1();           // start
   while ( 1 )
   {
      cp = (unsigned const char *)trc_err_dump2();   // get and display a line
      if ( cp == NULL )
         break;
      printf( "* %s\n", cp );
   }
   //pause();
getchar();
   // display the command history
   trc_cht_dump1();           // start
   while ( 1 )
   {
      cp = (unsigned const char *)trc_cht_dump2();   // get and display a line
      if ( cp == NULL )
         break;
      printf( "* %s\n", cp );
      lc ++ ;
      if ( ! ( lc & 0x000f ) )
      {
          //pause()
          ;
      }
   }

   // display the low level trace
   trc_llt_dump1();           // start
   while ( 1 )
   {
      cp = (unsigned const char *)trc_llt_dump2();   // get and display a line
      if ( cp == NULL )
         break;
      printf( "* %s\n", cp );
      lc ++ ;
      if ( ! ( lc & 0x000f ) )
      {
          //pause()
          ;
      }
   }

   if ( lc & 0x000f )
   {
       //pause()
       ;
   }
}



static void test_ide_io(void)
{
    //int rc;
    //int dev = 1;

#if 0
   {
       int secno = 0;
       int ndev = 1;
       physaddr_t physaddr;
       void *vaddr, *va2;

       if( hal_alloc_vaddress( &vaddr, 1 ) )
           panic("Pager alloc va failed");

       if( hal_alloc_vaddress( &va2, 1 ) )
           panic("Pager alloc va failed");

       if( hal_alloc_phys_page( &physaddr ) )
           panic("Pager alloc page failed");

       hal_page_control( physaddr, vaddr, page_map, page_rw );
       hal_page_control( physaddr, va2, page_map, page_rw );

       rc = dma_pci_lba28( ndev, CMD_READ_DMA,
                           0, 1, // feature reg, sect count
                           secno, 0, //physaddr,
                           1L );

       printf("\nIDE DMA rd test, rc=%d, phys 0x%lX, va2 = '%s'\n", rc, physaddr, va2 );

       strcpy(vaddr, "Ok, now test DMA IDE" );

       printf("IDE DMA wr test, phys 0x%lX, va2 = '%s'\n", physaddr, va2 );

       //ata->reg_incompat_flags |= REG_INCOMPAT_DMA_POLL | REG_INCOMPAT_DMA_DELAY;
       //ata->reg_incompat_flags |= REG_INCOMPAT_DMA_DELAY;

       ClearTrace();
       printf( "Test IO DMA LBA28...\n" );
       rc = dma_pci_lba28(  ndev, CMD_WRITE_DMA,
                            0, 1, // feature reg, sect count
                            secno, 0, //physaddr,
                            1L );
       printf("\nIDE DMA wr test, rc=%d, va2 = '%s'\n", rc, va2 );

       ShowAll();
       getchar();
   }
#endif

#if 0
   printf( "\n\nATAPI Identify, polling...\n" );
   memset( buffer, 0, sizeof( buffer ) );
   ClearTrace();
   rc = reg_pio_data_in_lba28(
               dev, CMD_IDENTIFY_DEVICE, //CMD_IDENTIFY_DEVICE_PACKET,
               0, 0,
               0L, //0L,
               buffer,
               1L, 0 );
   if ( rc )
   {
       printf("can't get identity, rc = %d", rc);
       ShowAll();
   }
   else
       hexdump( buffer, 256, "IDE id", 0 );

   printf("Press enter\n");
   getchar();
#endif

#if 0
   ClearTrace();
   printf( "Seek, LBA28, polling... " );
   rc = reg_non_data_lba28( dev, CMD_SEEK, 0, 0, 5025L );
   if ( rc )
   {
       ShowAll();
       printf("can't seek\n");
   }
   else
      printf("done\n");
#endif


#if 0
   // do an ATA Read Sectors command in LBA28 mode
   // lets read 3 sectors starting at LBA=5
   ClearTrace();
   printf( "ATA Read Sectors, LBA28, polling...\n" );
   memset( buffer, 0, sizeof( buffer ) );
   rc = reg_pio_data_in_lba28(
               dev, CMD_READ_SECTORS,
               0, 3,
               0L,
               buffer,
               3L, 0
               );
   if ( rc )
      ShowAll();
   else
   {
       hexdumpb(0, buffer, 256 );      //pause();
   }
#endif

}




//---------------------------------------------------------------------------

void phantom_check_disk_save_virtmem( void *start, int npages )
{
    printf("WILL WRITE VMEM TO DISK 0, PRESS Y\n");
    if( 'Y' != getchar() )
    {
        printf("Skipped\n");
        getchar();
        return;
    }
    printf("Writing mem... ");

    int diskp = 0;
    while( npages-- )
    {
        simple_ide_write_page(  start, kvtophys(start), diskp++, vmem_check_dev );
        start += 4096;

        if(!(diskp%32))
            printf("%d ", diskp );
    }
}



void phantom_check_disk_check_virtmem( void *_start, int npages )
{
    (void) _start;
    (void) npages;
#if 0
    char pagecopy[4096];
    char p0_pagecopy[4096];

    int nerr = 0;

    void *memp = _start;

    int diskp = 0;
    while( npages-- )
    {
        simple_ide_read_page(  pagecopy, diskp, vmem_check_dev );

        if(diskp == 0)
        {
            memcpy( p0_pagecopy, pagecopy, 4096 );
        }
        else
        {
            if( memcmp( p0_pagecopy, _start, 4096 ) )
            {
                printf("Aft reading blk %d page 0 is killed\n", diskp );
                getchar();
            }
        }

        if(!(diskp%32))
            printf("%d ", diskp );

        if( memcmp( pagecopy, memp, 4096) )
        {
            nerr++;

            char *dp = pagecopy, *mp = memp;
            int nbdiff = 0;
            int cnt;

            for( cnt = 0; cnt <4096; cnt++)
            {
                if( dp[cnt] != mp[cnt] )
                    nbdiff++;
            }
            printf("Block %d differs, %d bytes different, press Enter\n", diskp, nbdiff );
            getchar();
            while(1)
            {
                printf("Dump? Y/N\n");
                int c = getchar();
                getchar();
                if( c == 'N') break;
                if( c == 'Y')
                {
                    hexdumpb(0, memp, 256 );      printf("Press Enter\n");
                    //hexdumpb(0, start, 256 );      printf("Press Enter\n");
                    //hexdumpb(0, start, 256 );      printf("Press Enter\n");
                    //hexdumpb(0, start, 256 );      printf("Press Enter\n");
                    break;
                }

            }
        }

        memp += 4096;
        diskp++;
    }

    if(nerr)
    {
        printf("phantom_check_disk_check_virtmem: %d errors, press Enter\n", nerr);
        getchar();
    }
    else
        printf("phantom_check_disk_check_virtmem: mem is OK!\n");
#endif
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



errno_t simple_ide_idenify_device(int dev)
{
    SHOW_INFO( 0, "--- Identifying IDE %d: ", dev );

    u_int16_t buf[256];
    int rc = reg_pio_data_in_lba28(
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

#if 0
    if( !isATA )
    {
        printf("Not an ATA device\n");
        return ENXIO;
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

    disk_sectors[dev] = size;

    if( !(buf[49] & 0x0300) )
    {
        SHOW_ERROR0( 0, "Not LBA/DMA disk\n");
        return ENXIO;
    }

    return 0;
}





#if TEST_NEW_DISK_IO_STACK

#if !IDE_DMA
#error Just DMA
#endif

#include <disk.h>


static void d0_ide_write_page( long physaddr, long secno, int nsec )
{
    int ndev = 0;

    hal_mutex_lock( &ide_io );

    int i;
    for( i = nsec; i > 0; i-- )
    {
        int rc = dma_pci_lba28(
                               ndev, CMD_WRITE_DMA,
                               0, 1, // feature reg, sect count
                               secno, physaddr,
                               1L );
        if ( rc )
            panic("IDE write failure sec %ld", secno);
        secno++;
        physaddr += 512;

    }

    hal_mutex_unlock( &ide_io );
}

static void d0_ide_read_page( long physaddr, long secno, int nsec )
{
    int ndev = 0;
    int tries = 5;

    hal_mutex_lock( &ide_io );

retry:;
    int i;
    for( i = nsec; i > 0; i-- )
    {
        int rc = dma_pci_lba28(
                               ndev, CMD_READ_DMA,
                               0, 1, // feature reg, sect count
                               secno, physaddr,
                               1L );

        if ( rc )
        {
            if( tries-- <= 0 )
                panic("IDE read failure sec %ld", secno );
            else
            {
                printf("IDE read failure sec %ld, retry", secno );
                goto retry;
            }
        }
        secno++;
        physaddr += 512;
    }

    hal_mutex_unlock( &ide_io );
}


static pager_io_request *cur_rq;
static void dpc_func(void *a)
{
    (void) a;

    pager_io_request *rq = cur_rq;
    assert(rq);
    cur_rq = 0;

    if(rq->flag_pageout)
        d0_ide_write_page( rq->phys_page, rq->blockNo, rq->nSect );
    else if(rq->flag_pagein)
        d0_ide_read_page( rq->phys_page, rq->blockNo, rq->nSect );
    else
        panic("not rd not rw");

    //rq->pager_callback( rq, rq->flag_pagein );
    pager_io_request_done( rq );

}


errno_t simple_ide_AsyncIo( struct phantom_disk_partition *p, pager_io_request *rq )
{
    (void) p;

    // Does it syncronously in fact

    rq->flag_ioerror = 0;
    rq->rc = 0;

    assert( cur_rq == 0 );
    cur_rq = rq;
    dpc_request_trigger( &ide_dpc, 0);

    return 0;
}

phantom_disk_partition_t *phantom_create_simple_ide_partition_struct( long size )
{
    phantom_disk_partition_t * ret = phantom_create_partition_struct( 0, 0, size);

    ret->asyncIo = simple_ide_AsyncIo;
    ret->flags |= PART_FLAG_IS_WHOLE_DISK;


    //struct disk_q *q = calloc( 1, sizeof(struct disk_q) );
    //phantom_init_disk_q( q, startIoFunc );

    ret->specific = 0;
    strlcpy( ret->name, "IDE0", sizeof(ret->name) );

    //q->device = private;
    //q->unit = unit; // if this is multi-unit device, let 'em distinguish

    // errno_t phantom_register_disk_drive(ret);


    return ret;
}


void connect_ide_io(void)
{

    int size = disk_sectors[0];

    if(size <= 0)
    {
        SHOW_ERROR( 0, "Disk 0 size %d?", size );
        return;
    }


    phantom_disk_partition_t *p = phantom_create_simple_ide_partition_struct( size );
    if(p == 0)
    {
        SHOW_ERROR0( 0, "Failed to create whole disk partition" );
        return;
    }

    p->specific = (void *)-1; // must be not 0 for real disk

    errno_t err = phantom_register_disk_drive(p);
    if(err)
    {
        SHOW_ERROR( 0, "Disk 0 err %d", err );
        return;
    }

}
#else
void connect_ide_io(void) {}
#endif


#endif //!PAGING_PARTITION
#endif // ARCH_ia32
