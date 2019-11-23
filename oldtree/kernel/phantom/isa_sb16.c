#if 1
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Sound blaster driver.
 *
 *
**/


#define DEBUG_MSG_PREFIX "sb16"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

//#include "driver_map.h"

#include <ia32/pio.h>
#include <dev/isa/sb16_regs.h>
#include <sys/libkern.h>
#include <phantom_libc.h>
#include <time.h>

#include <hal.h>

//#include <x86/comreg.h>
#include <kernel/page.h>

#define BLOCK_LENGTH    512   /* Length of digitized sound output block     */
#define BUFFER_LENGTH 	BLOCK_LENGTH*2

#define BUFFER_PAGES 	(((BUFFER_LENGTH-1)/PAGE_SIZE)+1)

#define lo(value) (unsigned char)((value) & 0x00FF)
#define hi(value) (unsigned char)((value) >> 8)


typedef struct
{
    int active; // to break stall writes

    int dsp_ver_major;
    int dsp_ver_minor;

    int dma8;
    int dma16;

    int dma_maskport;
    int dma_clrptrport;
    int dma_modeport;
    int dma_addrport;
    int dma_countport;
    int dma_pageport;

    char dma_startmask;
    char dma_stopmask;
    char dma_mode;

    int resetport;
    int readport;
    int writeport;
    int pollport;
    int ackport;

    // Just SB16 and autoinit
    //int autoinit;
    //int sixteenbit;

    //! interrupts handled - TODO move to global dev code?
    int intcount;

    physaddr_t physbuf;

    int curblock;

    /* Addressing for auto-initialized transfers (Whole buffer)   */
    unsigned long buffer_addr;
    unsigned char buffer_page;
    unsigned int  buffer_ofs;

    void *	blockptr[2];
    /*
     Addressing for single-cycle transfers (One block at a time
    unsigned long block_addr[2];
    unsigned char block_page[2];
    unsigned int  block_ofs[2];
    */

    hal_spinlock_t      lock;

    //hal_mutex_t mutex; // unused?
    // write
    //hal_cond_t 	canWrite;

    hal_sem_t writeSema;

    volatile int 	avail4write;
    volatile void *	start4write;
} sb_t;







static int sb_start(phantom_device_t *dev); // Start device (begin using)
static int sb_stop(phantom_device_t *dev);  // Stop device

// Access from kernel - can block!
static int sb_read(struct phantom_device *dev, void *buf, int len);
static int sb_write(struct phantom_device *dev, const void *buf, int len);

static errno_t SB_Init(phantom_device_t * dev);
static int SB_ResetDSP(phantom_device_t *dev);

static void SB_WriteDSP(phantom_device_t *dev, u_int8_t value);
static u_int8_t SB_ReadDSP(phantom_device_t *dev);


//static void startblock_sc(phantom_device_t *dev);

static void sb_interrupt( void *_dev );

static errno_t sb_detect( phantom_device_t *dev );
static void hw_codec_read_dma_setup(phantom_device_t* dev);
static void hw_codec_read_irq_setup(phantom_device_t* dev);

static int hw_codec_reg_read(phantom_device_t* dev, u_int8_t index);



// TODO this is to be moved to driver_map.c and be kept in driver tables
static int seq_number = 0;

phantom_device_t * driver_isa_sb16_probe( int port, int irq, int stage )
{
    (void) stage;

    physaddr_t physbuf;
    if( hal_alloc_phys_pages_low( &physbuf, BUFFER_PAGES ) )
    {
        SHOW_ERROR( 0, "Can't allocate low mem, %d pages", BUFFER_PAGES );
        return 0;
    }

    phantom_device_t * dev = calloc(sizeof(phantom_device_t), 1);
    if(dev == 0)
        goto free1;

    dev->iobase = port;
    dev->irq = irq;

    dev->name = "SBlaster";
    dev->seq_number = seq_number++;

    dev->dops.start = sb_start;
    dev->dops.stop = sb_stop;

    dev->dops.read = sb_read;
    dev->dops.write = sb_write;

    if( hal_irq_alloc( irq, &sb_interrupt, dev, HAL_IRQ_SHAREABLE ) )
    {
        SHOW_ERROR( 0, "IRQ %d is busy", irq );
        goto free2;
    }

    sb_t *sb = calloc( sizeof(sb_t), 1 );
    if( sb == 0 ) goto free3;

    dev->drv_private = sb;

    sb->active = 0;

    //hal_mutex_init( &sb->mutex, "SB16" ); // unused?
    //hal_cond_init( &sb->canWrite, "SB16 Write" );

    hal_sem_init( &sb->writeSema, "SB16 Write" );

    sb->avail4write = 0;
    sb->start4write = 0;

    // No chance to set 'em? Kernel environment?
    sb->dma8 = 1;
    sb->dma16 = 5;

    SHOW_FLOW( 1, "@%x", dev->iobase );

    if( sb_detect(dev) )
    {
        SHOW_ERROR( 0, "SB16 not found at 0x%x", port );
        goto free4;
    }


    if( SB_Init(dev) )
    {
        SHOW_ERROR0( 0, "Unable to init SB16");
        goto free4;
    }

    SHOW_FLOW( 1, "SB16 found @%x", dev->iobase );
    hw_codec_read_dma_setup(dev);
    hw_codec_read_irq_setup(dev);

    sb->physbuf = physbuf;


    sb_start(dev);
    int rpt = 100;
    while(rpt--)
    {
        char meander[] = "zzzzzzzz        zzzzzzzz        zzzzzzzz        ";
        //char meander[] = "adgjlorzzroljgda                "; // really poor man's sine ;)
        sb_write(dev, meander, sizeof(meander));
    }
    sb_stop(dev);

    return dev;

    // TODO sema? mutex?
free4:
    free(sb);
free3:
    hal_irq_free( irq, &sb_interrupt, dev );
free2:
    free(dev);
free1:
    hal_free_phys_pages_low( physbuf, BUFFER_PAGES );
    return 0;

}




static errno_t SB_Init(phantom_device_t * dev)
{
    sb_t *sb = (sb_t *)(dev->drv_private);

    /* Sound card IO ports */
    sb->resetport  = dev->iobase + 0x006;
    sb->readport   = dev->iobase + 0x00A;
    sb->writeport  = dev->iobase + 0x00C;
    sb->pollport   = dev->iobase + 0x00E;

    SHOW_FLOW( 1, "Init @%x", dev->iobase );

    /* Reset DSP, get version, choose output mode */
    if(SB_ResetDSP(dev)) // TODO this reset is duplicate, remove?
        return ENXIO;

    SHOW_FLOW( 1, "Reset ok @%x", dev->iobase );

    SB_WriteDSP(dev,0xE1);  /* Get DSP version number */
    sb->dsp_ver_major = SB_ReadDSP(dev);
    sb->dsp_ver_minor = SB_ReadDSP(dev);

    //sb->autoinit   = (sb->dspversion > 2.0);

    int sixteenbit = (sb->dsp_ver_major >= 4) && (sb->dsp_ver_minor > 0) && (sb->dma16 != 0) && (sb->dma16 > 3);

    /* Compute DMA controller ports and parameters */
    if (!sixteenbit)
    {
        SHOW_ERROR0( 0, "SondBlaster - 8 bit?!");
        return ENXIO;
    }

    SHOW_FLOW( 1, "SB16 version %d.%d", sb->dsp_ver_major, sb->dsp_ver_minor );

    // TODO move DMA handling out
    sb->dma_maskport   = 0xD4;
    sb->dma_clrptrport = 0xD8;
    sb->dma_modeport   = 0xD6;
    sb->dma_addrport   = 0xC0 + 4*(sb->dma16-4);
    sb->dma_countport  = 0xC2 + 4*(sb->dma16-4);
    switch (sb->dma16)
    {
    case 5:        sb->dma_pageport = 0x8B;        break;
    case 6:        sb->dma_pageport = 0x89;        break;
    case 7:        sb->dma_pageport = 0x8A;        break;
    }
    sb->dma_stopmask  = sb->dma16-4 + 0x04;  /* 000001xx */
    sb->dma_startmask = sb->dma16-4 + 0x00;  /* 000000xx */
    //if(sb->autoinit)
    sb->dma_mode = sb->dma16-4 + 0x58;     /* 010110xx */
    //else        sb->dma_mode = sb->dma16-4 + 0x48;     /* 010010xx */
    sb->ackport = dev->iobase + 0x00F;

    return 0;
}





static void sb_interrupt( void *_dev )
{
    phantom_device_t * dev = _dev;

    int unit = dev->seq_number;
    //int addr = dev->iobase;

    sb_t *sb = (sb_t *)(dev->drv_private);

    SHOW_FLOW( 9, "sound blaster %d interrupt", unit );

    sb->intcount++;

    hal_sti();


    /* read the IRQ interrupt status register */
    int status = hw_codec_reg_read(dev, SB16_IRQ_STATUS);

    /* check if this hardware raised this interrupt */
    if (status & 0x03) {

        /* acknowledge DMA memory transfers */
        if (status & 0x01)   inb(dev->iobase + SB16_CODEC_ACK_8_BIT);
        if (status & 0x02)   inb(dev->iobase + SB16_CODEC_ACK_16_BIT);

#if 0
        /* handle buffer finished interrupt */
        if (((dev->playback_stream.bits >> 3) & status) != 0) {
            sb16_stream_buffer_done(&dev->playback_stream);
        }
        if (((dev->record_stream.bits >> 3) & status) != 0) {
            sb16_stream_buffer_done(&dev->record_stream);
        }

        if ((status & 0x04) != 0) {
            /* MIDI stream done */
        }
#endif        
    }



    // Clean buffer so that if no incoming data available,
    // No sound will be produced
    u_int16_t *destptr =  (u_int16_t *)sb->blockptr[sb->curblock];
    memset( destptr, 0, BLOCK_LENGTH*2 );

    int ie = hal_save_cli();
    hal_spin_lock( &sb->lock );
    sb->avail4write = BLOCK_LENGTH; // 16-bit words!
    sb->start4write = destptr;
    hal_spin_unlock( &sb->lock );
    if(ie) hal_sti();

    //hal_cond_signal( &sb->canWrite );
    hal_sem_release( &sb->writeSema );

    sb->curblock = !sb->curblock;  // Toggle block


}

/*
static void copy_sound16(void)
{
    int i;
    signed short *destptr;

    destptr = (short * __far)blockptr[curblock];

    for (i=0; i < BLOCK_LENGTH; i++)
        destptr[i] = mixingblock[i];
}
*/


static void start_dac(phantom_device_t *dev)
{
    sb_t *sb = (sb_t *)(dev->drv_private);

    //if (autoinit)
      { /* Auto init DMA */
        outb(sb->dma_maskport,   sb->dma_stopmask);
        outb(sb->dma_clrptrport, 0x00);
        outb(sb->dma_modeport,   sb->dma_mode);
        outb(sb->dma_addrport,   lo(sb->buffer_ofs));
        outb(sb->dma_addrport,   hi(sb->buffer_ofs));
        outb(sb->dma_countport,  lo(BUFFER_LENGTH-1));
        outb(sb->dma_countport,  hi(BUFFER_LENGTH-1));
        outb(sb->dma_pageport,   sb->buffer_page);
        outb(sb->dma_maskport,   sb->dma_startmask);
      }

    { /* Sixteen bit auto-initialized: SB16 and up (DSP 4.xx)             */
        SB_WriteDSP(dev, 0x41);                /* Set sound output sampling rate   */
        SB_WriteDSP(dev, hi(44100));
        SB_WriteDSP(dev, lo(44100));
        SB_WriteDSP(dev, 0xB6);                /* 16-bit cmd  - D/A - A/I - FIFO   */
        SB_WriteDSP(dev, 0x10);                /* 16-bit mode - signed mono        */
        SB_WriteDSP(dev, lo(BLOCK_LENGTH-1));
        SB_WriteDSP(dev, hi(BLOCK_LENGTH-1));
    }
  }

static void stop_dac(phantom_device_t *dev)
{
    sb_t *sb = (sb_t *)(dev->drv_private);

    SHOW_INFO( 1, "stop sb16 port = %x, unit %d", dev->iobase, dev->seq_number);

    SB_WriteDSP(dev,0xD5);                  /* Pause 16-bit DMA sound I/O       */
    outb(sb->dma_maskport, sb->dma_stopmask);   /* Stop DMA                         */
}




// Start device (begin using)
static int sb_start(phantom_device_t *dev)
{
    int addr = dev->iobase;
    int unit = dev->seq_number;

    sb_t *sb = (sb_t *)(dev->drv_private);

    //take_dev_irq(dev);
    SHOW_INFO( 1, "start sb16 port = %x, unit %d", addr, unit);

    static u_int16_t (*out16buf)[2][BLOCK_LENGTH] = NULL;

    out16buf = (u_int16_t (*)[2][512]) sb->physbuf;

    int i;
    for(i=0; i<2; i++)
        sb->blockptr[i] = &((*out16buf)[i]);

    /* DMA parameters */
    sb->buffer_addr = sb->physbuf;
    sb->buffer_page = sb->buffer_addr        / 65536;
    sb->buffer_ofs  = (sb->buffer_addr >> 1) % 65536;

    memset(out16buf, 0x00, BUFFER_LENGTH * sizeof(u_int16_t));

    sb->curblock = 0;
    sb->intcount = 0;

    sb->active = 1;
    start_dac(dev);

    return 0;
}

// Stop device
static int sb_stop(phantom_device_t *dev)
{
    //int addr = dev->iobase;
    sb_t *sb = (sb_t *)dev->drv_private;

    sb->active = 0;
    stop_dac(dev);

    return 0;
}




static int sb_read(struct phantom_device *dev, void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;
    return -EIO;
}

static int sb_write(struct phantom_device *dev, const void *buf, int len)
{
    sb_t *sb = (sb_t *)dev->drv_private;

    SHOW_FLOW( 1, "write len %d", len );

    while(len > 1)
    {
        // fictive
        //hal_mutex_lock(&sb->mutex);
        while(sb->avail4write == 0)
        {
            if( !sb->active )
                return -ENXIO;

            SHOW_FLOW( 1, "wait 4 buffer, sb->avail4write = %d", sb->avail4write );
            //hal_cond_timedwait( &sb->canWrite, &sb->mutex, 1000 );
            hal_sem_acquire_etc( &sb->writeSema, 1, SEM_FLAG_TIMEOUT, 1000000L );
        }
        //hal_mutex_unlock(&sb->mutex);

        SHOW_FLOW( 1, "write available %d", sb->avail4write );

        int ie = hal_save_cli();
        hal_spin_lock( &sb->lock );

        int avail = sb->avail4write;
        void * data = (void *)sb->start4write;

        assert(avail);
        assert(data);

        int tocopy = min( avail, len/2 );

        sb->avail4write -= tocopy;
        sb->start4write += tocopy;

        hal_spin_unlock( &sb->lock );
        if(ie) hal_sti();

        SHOW_FLOW( 1, "write tocopy %d", tocopy );

        memcpy( data, buf, tocopy * 2 );
        len -= tocopy*2;
        buf += tocopy*2;
    }

    return 0;
}








static void SB_WriteDSP(phantom_device_t *dev, u_int8_t value)
{
    sb_t *sb = (sb_t *)dev->drv_private;

    while ((inb(sb->writeport) & 0x80))
        ;
    outb(sb->writeport, value);
}

static u_int8_t  SB_ReadDSP(phantom_device_t *dev)
{
    sb_t *sb = (sb_t *)dev->drv_private;

    //SHOW_FLOW( 7, "rd dsp @ %x poll %x", sb->readport, sb->pollport );

    while (!(inb(sb->pollport) & 0x80))
        ;
    return(inb(sb->readport));
}

static errno_t SB_ResetDSP(phantom_device_t *dev)
{
    sb_t *sb = (sb_t *)dev->drv_private;

    SHOW_FLOW( 1, "reset @%x", sb->resetport );

    outb(sb->resetport, 1);
    //phantom_spinwait_msec(10);
    hal_sleep_msec(10);
    //hal_sleep_msec(100);
    outb(sb->resetport, 0);

    int i = 100;
    while(i-- > 0)
    {
        int status = SB_ReadDSP(dev);
        if( status == 0xAA )
            break;
        hal_sleep_msec(1);
        SHOW_FLOW( 11, "reset status = %x", status );
    }

    SHOW_FLOW( 1, "reset @%x i = %d", sb->resetport, i );

    return (i > 0) ? 0 : ENXIO;
}






















#if 1




static void
hw_codec_reg_write(phantom_device_t* dev, u_int8_t index, u_int8_t value)
{
    /* write a Mixer indirect register */
    outb(dev->iobase + SB16_MIXER_ADDRESS, index);
    outb(dev->iobase + SB16_MIXER_DATA, value);
}

static int
hw_codec_reg_read(phantom_device_t* dev, u_int8_t index)
{
    /* read a Mixer indirect register */
    outb(dev->iobase + SB16_MIXER_ADDRESS, index);
    return inb(dev->iobase + SB16_MIXER_DATA);
}



static void
hw_codec_read_irq_setup(phantom_device_t* dev)
{
    /* query the current IRQ line resource */
    int mask = hw_codec_reg_read(dev, SB16_IRQ_SETUP);

    //dev->irq = 5;
    uint32_t irq = 0;

    if (mask & 0x01)		irq = 2;
    if (mask & 0x02)		irq = 5;
    if (mask & 0x04)		irq = 7;
    if (mask & 0x08)		irq = 10;

    SHOW_INFO( 1, "Current IRQ in dev = %d", irq );

    //dev->irq = irq;
}

static void
hw_codec_read_dma_setup(phantom_device_t* dev)
{
    sb_t *sb = (sb_t *)dev->drv_private;

    /* query the current DMA channel resources */
    int mask = hw_codec_reg_read(dev, SB16_DMA_SETUP);

    sb->dma8 = 1;
    if (mask & 0x01)        sb->dma8 = 0;
    if (mask & 0x02)        sb->dma8 = 1;
    if (mask & 0x08)        sb->dma8 = 3;

    sb->dma16 = sb->dma8;
    if (mask & 0x20)        sb->dma16 = 5;
    if (mask & 0x40)        sb->dma16 = 6;
    if (mask & 0x80)        sb->dma16 = 7;

    SHOW_INFO( 1, "sb16 @%x dma8=%d, dma16=%d", dev->iobase, sb->dma8, sb->dma16 );

}

static void
hw_codec_write_irq_setup(phantom_device_t* dev)
{
    /* change programmable IRQ line resource */
    int mask = 0x02;

    if (dev->irq == 2)		mask = 0x01;
    if (dev->irq == 5)		mask = 0x02;
    if (dev->irq == 7)		mask = 0x04;
    if (dev->irq == 10)		mask = 0x08;

    hw_codec_reg_write(dev, SB16_IRQ_SETUP, mask);

    SHOW_INFO( 1, "Set dev IRQ to %d", dev->irq );
}

static void
hw_codec_write_dma_setup(phantom_device_t* dev)
{
    sb_t *sb = (sb_t *)dev->drv_private;
    /* change programmable DMA channel resources */
    hw_codec_reg_write(dev, SB16_DMA_SETUP, (1 << sb->dma8) | (1 << sb->dma16));
}









static void
hw_codec_write_byte(phantom_device_t *dev, u_int8_t value)
{
    int i;

    /* wait until the DSP is ready to receive data */
    for (i = 0; i < SB16_CODEC_IO_DELAY; i++) {
        if (!( inb(dev->iobase + SB16_CODEC_WRITE_STATUS) & 0x80))
            break;
    }

    /* write a byte to the DSP data port */
    outb(dev->iobase + SB16_CODEC_WRITE_DATA, value);
}

static int
hw_codec_read_byte(phantom_device_t *dev)
{
    int i;
    /* wait until the DSP has data available */
    for (i = 0; i < SB16_CODEC_IO_DELAY; i++) {
        if (inb(dev->iobase + SB16_CODEC_READ_STATUS) & 0x80)
            break;
    }

    /* read a byte from the DSP data port */
    return inb(dev->iobase + SB16_CODEC_READ_DATA);
}



static int
hw_codec_read_version(phantom_device_t *dev)
{
    int minor, major;

    // query the DSP hardware version number
    hw_codec_write_byte(dev, SB16_CODEC_VERSION);
    major = hw_codec_read_byte(dev);
    minor = hw_codec_read_byte(dev);

    SHOW_INFO( 0, "SB16 version %d.%d\n", major, minor);

    return (major << 8) + minor;
}


static errno_t
hw_codec_reset(phantom_device_t *dev)
{
    int times, delay;

    SHOW_FLOW( 1, "Reset @%x", dev->iobase );

    /* try to reset the DSP hardware */
    for (times = 0; times < 10; times++)
    {
        outb(dev->iobase + SB16_CODEC_RESET, 1);

        for (delay = 0; delay < SB16_CODEC_RESET_DELAY; delay++)
            inb(dev->iobase + SB16_CODEC_RESET);

        outb( dev->iobase + SB16_CODEC_RESET, 0);
        uint8_t ans = hw_codec_read_byte(dev);
        if( ans == 0xaa)
            return 0;

        SHOW_ERROR( 1, "Codec answer = @%x", ans );
    }

    return ENXIO;
}



static errno_t sb_detect( phantom_device_t *dev )
{
    errno_t rc;

    if ((rc=hw_codec_reset(dev)) == 0) {
        if (hw_codec_read_version(dev) >= 0x400) {
            hw_codec_write_irq_setup(dev);
            hw_codec_write_dma_setup(dev);
            rc = 0;
        } else {
            rc = ENXIO;
        }
    }

    return rc;
}
































#if 0



static int32
hw_codec_inth(void* cookie)
{
    phantom_device_t* dev = (phantom_device_t*)cookie;
    int32 rc = B_UNHANDLED_INTERRUPT;

    /* read the IRQ interrupt status register */
    int status = hw_codec_reg_read(dev, SB16_IRQ_STATUS);

    /* check if this hardware raised this interrupt */
    if (status & 0x03) {
        rc = B_HANDLED_INTERRUPT;

        /* acknowledge DMA memory transfers */
        if (status & 0x01)
            gISA->read_io_8(dev->iobase + SB16_CODEC_ACK_8_BIT);
        if (status & 0x02)
            gISA->read_io_8(dev->iobase + SB16_CODEC_ACK_16_BIT);

        /* acknowledge PIC interrupt signal */
        if (dev->irq >= 8)
            gISA->write_io_8(0xa0, 0x20);

        gISA->write_io_8(0x20, 0x20);

        /* handle buffer finished interrupt */
        if (((dev->playback_stream.bits >> 3) & status) != 0) {
            sb16_stream_buffer_done(&dev->playback_stream);
            rc = B_INVOKE_SCHEDULER;
        }
        if (((dev->record_stream.bits >> 3) & status) != 0) {
            sb16_stream_buffer_done(&dev->record_stream);
            rc = B_INVOKE_SCHEDULER;
        }

        if ((status & 0x04) != 0) {
            /* MIDI stream done */
            rc = B_INVOKE_SCHEDULER;
        }
    }

    return rc;
}





//#pragma mark -

status_t
sb16_stream_setup_buffers(phantom_device_t* dev, sb16_stream_t* s, const char* desc)
{
    return B_OK;
}

status_t
sb16_stream_start(phantom_device_t* dev, sb16_stream_t* s)
{
    return B_OK;
}

status_t
sb16_stream_stop(phantom_device_t* dev, sb16_stream_t* s)
{
    return B_OK;
}

void
sb16_stream_buffer_done(sb16_stream_t* stream)
{
}

//#pragma mark -

status_t
sb16_hw_init(phantom_device_t* dev)
{
    status_t rc;

    /* First of all, grab the ISA module */
    if ((rc=get_module(B_ISA_MODULE_NAME, (module_info**)&gISA)) != B_OK)
        return rc;

    /* Check if the hardware is sensible... */
    if ((rc=hw_codec_detect(dev)) == B_OK) {
        if ((rc=gISA->lock_isa_dma_channel(dev->dma8)) == B_OK &&
            (rc=gISA->lock_isa_dma_channel(dev->dma16)) == B_OK) {
            rc = install_io_interrupt_handler(dev->irq, hw_codec_inth, dev, 0);
        }
    }

    return rc;
}

void
sb16_hw_stop(phantom_device_t* dev)
{
}

void
sb16_hw_uninit(phantom_device_t* dev)
{
    remove_io_interrupt_handler(dev->irq, hw_codec_inth, dev);

    if (gISA != NULL) {
        gISA->unlock_isa_dma_channel(dev->dma8);

        if (dev->dma8 != dev->dma16)
            gISA->unlock_isa_dma_channel(dev->dma16);

        put_module(B_ISA_MODULE_NAME);
    }

}




























#include "driver.h"

multi_channel_info chans[] = {
    {  0, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 0 },
    {  1, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
    {  2, B_MULTI_INPUT_CHANNEL, 	B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 0 },
    {  3, B_MULTI_INPUT_CHANNEL, 	B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
    {  4, B_MULTI_OUTPUT_BUS, 		B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 	B_CHANNEL_MINI_JACK_STEREO },
    {  5, B_MULTI_OUTPUT_BUS, 		B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, B_CHANNEL_MINI_JACK_STEREO },
    {  6, B_MULTI_INPUT_BUS, 		B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 	B_CHANNEL_MINI_JACK_STEREO },
    {  7, B_MULTI_INPUT_BUS, 		B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, B_CHANNEL_MINI_JACK_STEREO },
};

static int32
format2size(uint32 format)
{
    switch(format) {
    case B_FMT_8BIT_S:
    case B_FMT_16BIT:
        return 2;

    default:
        return -1;
    }
}

static status_t
get_description(sb16_dev_t* dev, multi_description* data)
{
    data->interface_version = B_CURRENT_INTERFACE_VERSION;
    data->interface_minimum = B_CURRENT_INTERFACE_VERSION;

    strcpy(data->friendly_name,"SoundBlaster 16");
    strcpy(data->vendor_info,"Haiku");

    data->output_channel_count = 2;
    data->input_channel_count = 2;
    data->output_bus_channel_count = 2;
    data->input_bus_channel_count = 2;
    data->aux_bus_channel_count = 0;

    if (data->request_channel_count >= (int)(sizeof(chans) / sizeof(chans[0]))) {
        memcpy(data->channels,&chans,sizeof(chans));
    }

    /* determine output/input rates */
    data->output_rates =
        data->input_rates = B_SR_44100 | B_SR_22050 | B_SR_11025;

    data->max_cvsr_rate = 0;
    data->min_cvsr_rate = 0;

    data->output_formats =
        data->input_formats = B_FMT_8BIT_S | B_FMT_16BIT;
    data->lock_sources = B_MULTI_LOCK_INTERNAL;
    data->timecode_sources = 0;
    data->interface_flags = B_MULTI_INTERFACE_PLAYBACK | B_MULTI_INTERFACE_RECORD;
    data->start_latency = 30000;

    strcpy(data->control_panel,"");

    return B_OK;
}

static status_t
get_enabled_channels(sb16_dev_t* dev, multi_channel_enable* data)
{
    B_SET_CHANNEL(data->enable_bits, 0, true);
    B_SET_CHANNEL(data->enable_bits, 1, true);
    B_SET_CHANNEL(data->enable_bits, 2, true);
    B_SET_CHANNEL(data->enable_bits, 3, true);
    data->lock_source = B_MULTI_LOCK_INTERNAL;

    return B_OK;
}

static status_t
get_global_format(sb16_dev_t* dev, multi_format_info* data)
{
    data->output_latency = 0;
    data->input_latency = 0;
    data->timecode_kind = 0;

    data->output.format = dev->playback_stream.sampleformat;
    data->output.rate = dev->playback_stream.samplerate;

    data->input.format = dev->record_stream.sampleformat;
    data->input.rate = dev->record_stream.samplerate;

    return B_OK;
}

static status_t
set_global_format(sb16_dev_t* dev, multi_format_info* data)
{
    dev->playback_stream.sampleformat = data->output.format;
    dev->playback_stream.samplerate = data->output.rate;
    dev->playback_stream.sample_size = format2size(dev->playback_stream.sampleformat);

    dev->record_stream.sampleformat = data->input.format;
    dev->record_stream.samplerate = data->input.rate;
    dev->record_stream.sample_size = format2size(dev->record_stream.sampleformat);

    return B_OK;
}

static int32
create_group_control(multi_mix_control* multi, int32 idx, int32 parent, int32 string, const char* name)
{
    multi->id = SB16_MULTI_CONTROL_FIRSTID + idx;
    multi->parent = parent;
    multi->flags = B_MULTI_MIX_GROUP;
    multi->master = SB16_MULTI_CONTROL_MASTERID;
    multi->string = string;
    if(name)
        strcpy(multi->name, name);

    return multi->id;
}

static status_t
list_mix_controls(sb16_dev_t* dev, multi_mix_control_info * data)
{
    int32 parent;

    parent = create_group_control(data->controls +0, 0, 0, 0, "Record");
    parent = create_group_control(data->controls +1, 1, 0, 0, "AC97 Mixer");
    parent = create_group_control(data->controls +2, 2, 0, S_SETUP, NULL);
    data->control_count = 3;

    return B_OK;
}

static status_t
list_mix_connections(sb16_dev_t* dev, multi_mix_connection_info * data)
{
    return B_ERROR;
}

static status_t
list_mix_channels(sb16_dev_t* dev, multi_mix_channel_info *data)
{
    return B_ERROR;
}

static status_t
get_buffers(sb16_dev_t* dev, multi_buffer_list* data)
{
    uint32 playback_sample_size = dev->playback_stream.sample_size;
    uint32 record_sample_size = dev->record_stream.sample_size;
    uint32 cidx, bidx;
    status_t rc;

    dprintf("%s: playback: %ld buffers, %ld channels, %ld samples\n", __func__,
            data->request_playback_buffers, data->request_playback_channels, data->request_playback_buffer_size);
    dprintf("%s: record: %ld buffers, %ld channels, %ld samples\n", __func__,
            data->request_record_buffers, data->request_record_channels, data->request_record_buffer_size);

    /* Workaround for Haiku multi_audio API, since it prefers to let the driver pick
     values, while the BeOS multi_audio actually gives the user's defaults. */
    if (data->request_playback_buffers > STRMAXBUF ||
        data->request_playback_buffers < STRMINBUF) {
        data->request_playback_buffers = STRMINBUF;
    }

    if (data->request_record_buffers > STRMAXBUF ||
        data->request_record_buffers < STRMINBUF) {
        data->request_record_buffers = STRMINBUF;
    }

    if (data->request_playback_buffer_size == 0)
        data->request_playback_buffer_size  = DEFAULT_FRAMESPERBUF;

    if (data->request_record_buffer_size == 0)
        data->request_record_buffer_size  = DEFAULT_FRAMESPERBUF;

    /* ... from here on, we can assume again that a reasonable request is being made */

    data->flags = 0;

    /* Copy the requested settings into the streams */
    dev->playback_stream.num_buffers = data->request_playback_buffers;
    dev->playback_stream.num_channels = data->request_playback_channels;
    dev->playback_stream.buffer_length = data->request_playback_buffer_size;
    if ((rc=sb16_stream_setup_buffers(dev, &dev->playback_stream, "Playback")) != B_OK) {
        dprintf("%s: Error setting up playback buffers (%s)\n", __func__, strerror(rc));
        return rc;
    }

    dev->record_stream.num_buffers = data->request_record_buffers;
    dev->record_stream.num_channels = data->request_record_channels;
    dev->record_stream.buffer_length = data->request_record_buffer_size;
    if ((rc=sb16_stream_setup_buffers(dev, &dev->record_stream, "Recording")) != B_OK) {
        dprintf("%s: Error setting up recording buffers (%s)\n", __func__, strerror(rc));
        return rc;
    }

    /* Setup data structure for multi_audio API... */
    data->return_playback_buffers = data->request_playback_buffers;
    data->return_playback_channels = data->request_playback_channels;
    data->return_playback_buffer_size = data->request_playback_buffer_size;		/* frames */

    for (bidx=0; bidx < data->return_playback_buffers; bidx++) {
        for (cidx=0; cidx < data->return_playback_channels; cidx++) {
            data->playback_buffers[bidx][cidx].base = dev->playback_stream.buffers[bidx] + (playback_sample_size * cidx);
            data->playback_buffers[bidx][cidx].stride = playback_sample_size * data->return_playback_channels;
        }
    }

    data->return_record_buffers = data->request_record_buffers;
    data->return_record_channels = data->request_record_channels;
    data->return_record_buffer_size = data->request_record_buffer_size;			/* frames */

    for (bidx=0; bidx < data->return_record_buffers; bidx++) {
        for (cidx=0; cidx < data->return_record_channels; cidx++) {
            data->record_buffers[bidx][cidx].base = dev->record_stream.buffers[bidx] + (record_sample_size * cidx);
            data->record_buffers[bidx][cidx].stride = record_sample_size * data->return_record_channels;
        }
    }

    return B_OK;
}

static status_t
buffer_exchange(sb16_dev_t* dev, multi_buffer_info* data)
{
    static int debug_buffers_exchanged = 0;
    cpu_status status;
    status_t rc;

    if (!dev->playback_stream.running)
        sb16_stream_start(dev, &dev->playback_stream);

    /* do playback */
    rc=acquire_sem(dev->playback_stream.buffer_ready_sem);
    if (rc != B_OK) {
        dprintf("%s: Error waiting for playback buffer to finish (%s)!\n", __func__,
                strerror(rc));
        return rc;
    }

    status = disable_interrupts();
    acquire_spinlock(&dev->playback_stream.lock);

    data->playback_buffer_cycle = dev->playback_stream.buffer_cycle;
    data->played_real_time = dev->playback_stream.real_time;
    data->played_frames_count = dev->playback_stream.frames_count;

    release_spinlock(&dev->playback_stream.lock);
    restore_interrupts(status);

    debug_buffers_exchanged++;
    if (((debug_buffers_exchanged % 100) == 1) && (debug_buffers_exchanged < 1111)) {
        dprintf("%s: %d buffers processed\n", __func__, debug_buffers_exchanged);
    }

    return B_OK;
}

static status_t
buffer_force_stop(sb16_dev_t* dev)
{
    sb16_stream_stop(dev, &dev->playback_stream);
    sb16_stream_stop(dev, &dev->record_stream);

    delete_sem(dev->playback_stream.buffer_ready_sem);
    delete_sem(dev->record_stream.buffer_ready_sem);

    return B_OK;
}

status_t
multi_audio_control(void* cookie, uint32 op, void* arg, size_t len)
{
    switch(op) {
    case B_MULTI_GET_DESCRIPTION:			return get_description(cookie, arg);
    case B_MULTI_GET_EVENT_INFO:			return B_ERROR;
    case B_MULTI_SET_EVENT_INFO:			return B_ERROR;
    case B_MULTI_GET_EVENT:					return B_ERROR;
    case B_MULTI_GET_ENABLED_CHANNELS:		return get_enabled_channels(cookie, arg);
    case B_MULTI_SET_ENABLED_CHANNELS:		return B_OK;
    case B_MULTI_GET_GLOBAL_FORMAT:			return get_global_format(cookie, arg);
    case B_MULTI_SET_GLOBAL_FORMAT:			return set_global_format(cookie, arg);
    case B_MULTI_GET_CHANNEL_FORMATS:		return B_ERROR;
    case B_MULTI_SET_CHANNEL_FORMATS:		return B_ERROR;
    case B_MULTI_GET_MIX:					return B_ERROR;
    case B_MULTI_SET_MIX:					return B_ERROR;
    case B_MULTI_LIST_MIX_CHANNELS:			return list_mix_channels(cookie, arg);
    case B_MULTI_LIST_MIX_CONTROLS:			return list_mix_controls(cookie, arg);
    case B_MULTI_LIST_MIX_CONNECTIONS:		return list_mix_connections(cookie, arg);
    case B_MULTI_GET_BUFFERS:				return get_buffers(cookie, arg);
    case B_MULTI_SET_BUFFERS:				return B_ERROR;
    case B_MULTI_SET_START_TIME:			return B_ERROR;
    case B_MULTI_BUFFER_EXCHANGE:			return buffer_exchange(cookie, arg);
    case B_MULTI_BUFFER_FORCE_STOP:			return buffer_force_stop(cookie);
    }

    return B_BAD_VALUE;
}
#endif


#endif


#endif
