/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Sound blaster driver.
 *
 *
**/


#define DEBUG_MSG_PREFIX "sb"
#include "debug_ext.h"
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include "driver_map.h"

#include <i386/pio.h>
#include <phantom_libc.h>
#include <time.h>

#include "hal.h"

#include <x86/comreg.h>
#include <x86/phantom_page.h>

#define BLOCK_LENGTH    512   /* Length of digitized sound output block     */
#define BUFFER_LENGTH 	BLOCK_LENGTH*2

#define lo(value) (unsigned char)((value) & 0x00FF)
#define hi(value) (unsigned char)((value) >> 8)


typedef struct
{
    int active; // to break stall writes

    float dspversion;

    int dma;
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

    hal_mutex_t mutex; // unused?

    // write
    hal_cond_t 	canWrite;
    int 	avail4write;
    void *	start4write;
} sb_t;




static int
sb_detect(int unit, int addr)
{
    return 0;
}



static int sb_start(phantom_device_t *dev); // Start device (begin using)
static int sb_stop(phantom_device_t *dev);  // Stop device

// Access from kernel - can block!
static int sb_read(struct phantom_device *dev, void *buf, int len);
static int sb_write(struct phantom_device *dev, const void *buf, int len);

static errno_t SB_Init(phantom_device_t * dev);
static int SB_ResetDSP(phantom_device_t *dev);

static void SB_WriteDSP(phantom_device_t *dev, u_int8_t value);
static u_int8_t SB_ReadDSP(phantom_device_t *dev);


static void startblock_sc(phantom_device_t *dev);

static void sb_interrupt( void *_dev );

// TODO this is to be moved to driver_map.c and be kept in driver tables
static int seq_number = 0;

phantom_device_t * driver_isa_sb_probe( int port, int irq, int stage )
{
    if( !sb_detect( seq_number, port) )
        return 0;

    physaddr_t physbuf;
    if( hal_alloc_phys_pages_low( &physbuf, ((BUFFER_LENGTH-1)/PAGE_SIZE)+1 ) )
        return 0;


    phantom_device_t * dev = calloc(sizeof(phantom_device_t), 1);

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
        free(dev);
        return 0;
    }

    sb_t *sb = calloc( sizeof(sb_t), 1 );
    dev->drv_private = sb;

    sb->active = 0;

    hal_mutex_init( &sb->mutex, "SB16" ); // unused?

    hal_cond_init( &sb->canWrite, "SB16 Write" );
    sb->avail4write = 0;
    sb->start4write = 0;

    if( SB_Init(dev) )
    {
        free(sb);
        free(dev);
        hal_irq_free( irq, &sb_interrupt, dev );
        SHOW_ERROR0( 0, "Unable to init SB16");
    }

    sb->physbuf = physbuf;

    return dev;
}




static errno_t SB_Init(phantom_device_t * dev)
{
    sb_t *sb = (sb_t *)(dev->drv_private);

    /* Sound card IO ports */
    sb->resetport  = dev->iobase + 0x006;
    sb->readport   = dev->iobase + 0x00A;
    sb->writeport  = dev->iobase + 0x00C;
    sb->pollport   = dev->iobase + 0x00E;

    /* Reset DSP, get version, choose output mode */
    if (!SB_ResetDSP(dev))
        return ENXIO;

    SB_WriteDSP(dev,0xE1);  /* Get DSP version number */
    sb->dspversion = SB_ReadDSP(dev);  sb->dspversion += SB_ReadDSP(dev) / 100.0;
    //sb->autoinit   = (sb->dspversion > 2.0);
    int sixteenbit = (sb->dspversion > 4.0) && (sb->dma16 != 0) && (sb->dma16 > 3);

    /* Compute DMA controller ports and parameters */
    if (!sixteenbit)
    {
        SHOW_ERROR0( 0, "SondBlaster - 8 bit?!");
        return ENXIO;
    }

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
    int addr = dev->iobase;

    sb_t *sb = (sb_t *)(dev->drv_private);

    SHOW_FLOW( 9, "sound blaster %d interrupt", unit );

    sb->intcount++;

    hal_sti();
    /*
    if(!sb->autoinit)   // Start next block quickly if not using auto-init DMA
    {
        startblock_sc(dev);
        copy_sound();
        sb->curblock = !sb->curblock;  // Toggle block
    }*/

    //update_voices();
    //mix_voices();

    //if(sb->autoinit)
    {
        //copy_sound();

        // Clean buffer so that if no incoming data available,
        // No sound will be produced
        u_int16_t *destptr =  (u_int16_t *)sb->blockptr[sb->curblock];
        memset( destptr, BLOCK_LENGTH*2, 0 );

        int ie = hal_save_cli();
        hal_spin_lock( &sb->lock );
        sb->avail4write = BLOCK_LENGTH; // 16-bit words!
        sb->start4write = destptr;
        hal_spin_unlock( &sb->lock );
        if(ie) hal_sti();

        hal_cond_signal( &sb->canWrite );

        sb->curblock = !sb->curblock;  // Toggle block
    }

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
    int addr = dev->iobase;
    sb_t *sb = (sb_t *)dev->drv_private;

    sb->active = 0;
    stop_dac(dev);

    return 0;
}




static int sb_read(struct phantom_device *dev, void *buf, int len)
{
    return -EIO;
}

static int sb_write(struct phantom_device *dev, const void *buf, int len)
{
    sb_t *sb = (sb_t *)dev->drv_private;

    while(len > 1)
    {
        while(sb->avail4write == 0)
        {
            if( !sb->active )
                return -ENXIO;

            hal_cond_timedwait( &sb->canWrite, &sb->mutex, 1000 );
        }

        int ie = hal_save_cli();
        hal_spin_lock( &sb->lock );

        int avail = sb->avail4write;
        void * data = sb->start4write;

        assert(avail);
        assert(data);

        int tocopy = min( avail, len/2 );

        sb->avail4write -= tocopy;
        sb->start4write += tocopy;

        hal_spin_unlock( &sb->lock );
        if(ie) hal_sti();

        memcpy( data, buf, tocopy * 2 );
        len -= tocopy*2;
        buf += tocopy*2;
    }

    return 0;
}








static void SB_WriteDSP(phantom_device_t *dev, u_int8_t value)
  {
    sb_t *sb = (sb_t *)dev->drv_private;

    while ((inp(sb->writeport) & 0x80))
        ;
    outb(sb->writeport, value);
  }

static u_int8_t  SB_ReadDSP(phantom_device_t *dev)
{
    sb_t *sb = (sb_t *)dev->drv_private;

    while (!(inp(sb->pollport) & 0x80))
        ;
    return(inp(sb->readport));
}

static errno_t SB_ResetDSP(phantom_device_t *dev)
  {
    int i;

    sb_t *sb = (sb_t *)dev->drv_private;

    outb(sb->resetport, 1);
    //phantom_spinwait_msec(10);
    hal_sleep_msec(10);
    outb(sb->resetport, 0);

    i = 100;
    while ((i-- > 0) && (SB_ReadDSP(dev) != 0xAA))
        ;

    return (i > 0) ? 0 : ENXIO;
  }


/*
//! Starts a single-cycle DMA transfer
static void startblock_sc(phantom_device_t *dev)
{
    sb_t *sb = (sb_t *)dev->drv_private;

    outb(sb->dma_maskport,   sb->dma_stopmask);
    outb(sb->dma_clrptrport, 0x00);
    outb(sb->dma_modeport,   sb->dma_mode);
    outb(sb->dma_addrport,   lo(sb->block_ofs[sb->curblock]));
    outb(sb->dma_addrport,   hi(sb->block_ofs[sb->curblock]));
    outb(sb->dma_countport,  lo(BLOCK_LENGTH-1));
    outb(sb->dma_countport,  hi(BLOCK_LENGTH-1));
    outb(sb->dma_pageport,   sb->block_page[sb->curblock]);
    outb(sb->dma_maskport,   sb->dma_startmask);

    SB_WriteDSP(dev, 0x14);                // 8-bit single-cycle DMA sound output
    SB_WriteDSP(dev, lo(BLOCK_LENGTH-1));
    SB_WriteDSP(dev, hi(BLOCK_LENGTH-1));
}
*/




#if 0




// SOUNDBLASTER 8-CHANNEL MIXING LIBRARY
// Remember to compile SBLIB.CPP with the -ZU option!

#ifndef _SBLIB_H_
#define _SBLIB_H_

#include <stdio.h>

// Sample type

typedef struct
{
 signed char *data;
 unsigned long size;
} SB_Sample;

// Tries to init soundblaster

int SB_Init(int baseio, int irq, int dma, int dma16);

// Removes handler and resets DSP

void SB_Done();

// Allocate buffers and start sound output

void SB_MixOn();

// De-allocate buffers and stop sound ouput

void SB_MixOff();

// Load RAW sample

void SB_LoadRAW(FILE * handle,SB_Sample **sound);

// Free sound from memory

void SB_FreeSound(SB_Sample **sound);

// Start a sound

void SB_PlaySound(SB_Sample *sound, int index, unsigned char volume, int loop);

// Stop a sound

void SB_StopSound(int index);

// Check if a sound is currently playing

int SB_PlayCheck(int index);

// Sets overall sound volume

void SB_SetVol(unsigned char new_volume);

extern volatile long intcount;
extern volatile int voicecount;

extern float dspversion;

#endif


// 8-chan mixing lib

#define TRUE  1
#define FALSE 0
#define ON  1
#define OFF 0

#define BLOCK_LENGTH    512   /* Length of digitized sound output block     */
#define VOICES          8     /* Number of available simultaneous voices    */
#define VOLUMES         64    /* Number of volume levels for sound output   */

volatile long intcount;               /* Current count of sound interrupts  */
volatile int  voicecount;             /* Number of voices currently in use  */

float dspversion;
int   autoinit;
int   sixteenbit;

#include <dos.h>
#include <i86.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <malloc.h>
#include <mem.h>
#include "sblib.h"
#include "sbstuff.h"

#define BUFFER_LENGTH BLOCK_LENGTH*2

#define BYTE unsigned char

#define lo(value) (unsigned char)((value) & 0x00FF)
#define hi(value) (unsigned char)((value) >> 8)

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) > (b)) ? (b) : (a))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) > (b)) ? (b) : (a))

static int resetport;
static int readport;
static int writeport;
static int pollport;
static int ackport;

static int pic_rotateport;
static int pic_maskport;

static int dma_maskport;
static int dma_clrptrport;
static int dma_modeport;
static int dma_addrport;
static int dma_countport;
static int dma_pageport;

static char irq_startmask;
static char irq_stopmask;
static char irq_intvector;

static char dma_startmask;
static char dma_stopmask;
static char dma_mode;

static void (interrupt far *oldintvector)(void);

static int handler_installed;

void SB_LoadWav(FILE * handle, SB_Sample **sound)
{

}



void install_handler(void);
void uninstall_handler(void);
void smix_exitproc(void);

int SB_Init(int baseio, int irq, int dma, int dma16)
  {
   /* Sound card IO ports */
    resetport  = baseio + 0x006;
    readport   = baseio + 0x00A;
    writeport  = baseio + 0x00C;
    pollport   = baseio + 0x00E;

   /* Reset DSP, get version, choose output mode */
    if (!SB_ResetDSP())
      return(FALSE);
    SB_WriteDSP(0xE1);  /* Get DSP version number */
    dspversion = SB_ReadDSP();  dspversion += SB_ReadDSP() / 100.0;
    autoinit   = (dspversion > 2.0);
    sixteenbit = (dspversion > 4.0) && (dma16 != 0) && (dma16 > 3);

   /* Compute interrupt controller ports and parameters */
    if (irq < 8)
      { /* PIC1 */
        irq_intvector  = 0x08 + irq;
        pic_rotateport = 0x20;
        pic_maskport   = 0x21;
      }
    else
      { /* PIC2 */
        irq_intvector  = 0x70 + irq-8;
        pic_rotateport = 0xA0;
        pic_maskport   = 0xA1;
      }
    irq_stopmask  = 1 << (irq % 8);
    irq_startmask = ~irq_stopmask;

   /* Compute DMA controller ports and parameters */
    if (sixteenbit)
      { /* Sixteen bit */
        dma_maskport   = 0xD4;
        dma_clrptrport = 0xD8;
        dma_modeport   = 0xD6;
        dma_addrport   = 0xC0 + 4*(dma16-4);
        dma_countport  = 0xC2 + 4*(dma16-4);
        switch (dma16)
          {
            case 5:
              dma_pageport = 0x8B;
              break;
            case 6:
              dma_pageport = 0x89;
              break;
            case 7:
              dma_pageport = 0x8A;
              break;
          }
        dma_stopmask  = dma16-4 + 0x04;  /* 000001xx */
        dma_startmask = dma16-4 + 0x00;  /* 000000xx */
        if (autoinit)
          dma_mode = dma16-4 + 0x58;     /* 010110xx */
        else
          dma_mode = dma16-4 + 0x48;     /* 010010xx */
        ackport = baseio + 0x00F;
      }
    else
      { /* Eight bit */
        dma_maskport   = 0x0A;
        dma_clrptrport = 0x0C;
        dma_modeport   = 0x0B;
        dma_addrport   = 0x00 + 2*dma;
        dma_countport  = 0x01 + 2*dma;
        switch (dma)
          {
            case 0:
              dma_pageport = 0x87;
              break;
            case 1:
              dma_pageport = 0x83;
              break;
            case 2:
              dma_pageport = 0x81;
              break;
            case 3:
              dma_pageport = 0x82;
              break;
          }
        dma_stopmask  = dma + 0x04;      /* 000001xx */
        dma_startmask = dma + 0x00;      /* 000000xx */
        if (autoinit)
          dma_mode    = dma + 0x58;      /* 010110xx */
        else
          dma_mode    = dma + 0x48;      /* 010010xx */
        ackport = baseio + 0x00E;
      }
    install_handler();
    atexit(smix_exitproc);
    return(TRUE);
  }

void SB_Done(void)
  {
    if (handler_installed) uninstall_handler();
    SB_ResetDSP();
  }

/* Voice control */

typedef struct
  {
    SB_Sample *sound;
    int   index;
    int   volume;
    int   loop;
    long  curpos;
    int   done;
  } VOICE;

static int   inuse[VOICES];
static VOICE voice[VOICES];



/* Volume lookup table */
static signed int (*volume_table)[VOLUMES][256];

/* Mixing buffer */
static signed int  mixingblock[BLOCK_LENGTH];  /* Signed 16 bit */

/* Output buffers */
static void          (*outmemarea)                = NULL;
static unsigned char (*out8buf)[2][BLOCK_LENGTH]  = NULL;
static signed  short (*out16buf)[2][BLOCK_LENGTH] = NULL;

static void *blockptr[2];

static short int outmemarea_sel;              /* Selector for output buffer */

/* Addressing for auto-initialized transfers (Whole buffer)   */
static unsigned long buffer_addr;
static unsigned char buffer_page;
static unsigned int  buffer_ofs;

/* Addressing for single-cycle transfers (One block at a time */
static unsigned long block_addr[2];
static unsigned char block_page[2];
static unsigned int  block_ofs[2];

//static int handler_installed;

static unsigned char sound_volume;

/* 8-bit clipping */

static unsigned char (*clip_8_buf)[256*VOICES];
static unsigned char (*clip_8)[256*VOICES];

static void start_dac(void)
  {
    if (autoinit)
      { /* Auto init DMA */
        outb(dma_maskport,   dma_stopmask);
        outb(dma_clrptrport, 0x00);
        outb(dma_modeport,   dma_mode);
        outb(dma_addrport,   lo(buffer_ofs));
        outb(dma_addrport,   hi(buffer_ofs));
        outb(dma_countport,  lo(BUFFER_LENGTH-1));
        outb(dma_countport,  hi(BUFFER_LENGTH-1));
        outb(dma_pageport,   buffer_page);
        outb(dma_maskport,   dma_startmask);
      }
    else
      { /* Single cycle DMA */
        outb(dma_maskport,   dma_stopmask);
        outb(dma_clrptrport, 0x00);
        outb(dma_modeport,   dma_mode);
        outb(dma_addrport,   lo(buffer_ofs));
        outb(dma_addrport,   hi(buffer_ofs));
        outb(dma_countport,  lo(BLOCK_LENGTH-1));
        outb(dma_countport,  hi(BLOCK_LENGTH-1));
        outb(dma_pageport,   buffer_page);
        outb(dma_maskport,   dma_startmask);
      }

    if (sixteenbit)
      { /* Sixteen bit auto-initialized: SB16 and up (DSP 4.xx)             */
        SB_WriteDSP(0x41);                /* Set sound output sampling rate   */
        SB_WriteDSP(hi(22050));
        SB_WriteDSP(lo(22050));
        SB_WriteDSP(0xB6);                /* 16-bit cmd  - D/A - A/I - FIFO   */
        SB_WriteDSP(0x10);                /* 16-bit mode - signed mono        */
        SB_WriteDSP(lo(BLOCK_LENGTH-1));
        SB_WriteDSP(hi(BLOCK_LENGTH-1));
      }
    else
      { /* Eight bit */
        if (autoinit)
          { /* Eight bit auto-initialized:  SBPro and up (DSP 2.00+)        */
            SB_WriteDSP(0xD1);            /* Turn on speaker                  */
            SB_WriteDSP(0x40);            /* Set sound output time constant   */
            SB_WriteDSP(211);             /*  = 256 - (1000000 / rate)        */
            SB_WriteDSP(0x48);            /* Set DSP block transfer size      */
            SB_WriteDSP(lo(BLOCK_LENGTH-1));
            SB_WriteDSP(hi(BLOCK_LENGTH-1));
            SB_WriteDSP(0x1C);            /* 8-bit auto-init DMA mono output  */
          }
        else
          { /* Eight bit single-cycle:  Sound Blaster (DSP 1.xx+)           */
            SB_WriteDSP(0xD1);            /* Turn on speaker                  */
            SB_WriteDSP(0x40);            /* Set sound output time constant   */
            SB_WriteDSP(211);             /*  = 256 - (1000000 / rate)        */
            SB_WriteDSP(0x14);            /* 8-bit single-cycle DMA output    */
            SB_WriteDSP(lo(BLOCK_LENGTH-1));
            SB_WriteDSP(hi(BLOCK_LENGTH-1));
          }
      }
  }

static void stop_dac(void)
  {
    if (sixteenbit)
      SB_WriteDSP(0xD5);                  /* Pause 16-bit DMA sound I/O       */
    else
      {
        SB_WriteDSP(0xD0);                /* Pause 8-bit DMA sound I/O        */
        SB_WriteDSP(0xD3);                /* Turn off speaker                 */
      }

    outb(dma_maskport, dma_stopmask);   /* Stop DMA                         */
  }

/* Volume control */

static void init_volume_table(void)
  {
    unsigned int  volume;
    signed   int  insample;
    signed   char invalue;

    volume_table = (int (* )[64][256])malloc(VOLUMES * 256 * sizeof(signed int));

    for (volume=0; volume < VOLUMES; volume++)
      for (insample = -128; insample <= 127; insample++)
        {
          invalue = insample;
          (*volume_table)[volume][(unsigned char)invalue] =
            (((float)volume/(float)(VOLUMES-1)) * 32 * invalue);
        }

    sound_volume = 255;
  }

void SB_SetVol(unsigned char new_volume)
  {
    sound_volume = new_volume;
  }

/* Mixing initialization */

static void init_clip8(void)
  {
    int i;
    int value;

    clip_8_buf = (unsigned char (* )[2048])malloc(256*VOICES);
    clip_8     = (unsigned char (* )[2048])clip_8_buf + 128*VOICES;

    for (i = -128*VOICES; i < 128*VOICES; i++)
      {
        value = i;
        value = max(value, -128);
        value = min(value, 127);

        (*clip_8)[i] = value + 128;
      }
  }

static unsigned long linear_addr(void *ptr)
  {
    return((unsigned long)(ptr));
  }

void deallocate_voice(int voicenum);

void SB_MixOn(void)
  {
    int i;

    for (i=0; i < VOICES; i++)
      deallocate_voice(i);
    voicecount = 0;

    if (sixteenbit)
      {
       /* Find a block of memory that does not cross a page boundary */
        outmemarea=low_malloc(4*BUFFER_LENGTH, &outmemarea_sel);
        out16buf=(short (* )[2][512])outmemarea;
        if ((((linear_addr(out16buf) >> 1) % 65536) + BUFFER_LENGTH) > 65536)
          out16buf += BUFFER_LENGTH;

        for (i=0; i<2; i++)
          blockptr[i] = &((*out16buf)[i]);

       /* DMA parameters */
        buffer_addr = linear_addr(out16buf);
        buffer_page = buffer_addr        / 65536;
        buffer_ofs  = (buffer_addr >> 1) % 65536;

        memset(out16buf, 0x00, BUFFER_LENGTH * sizeof(signed short));
      }
    else
      {
       /* Find a block of memory that does not cross a page boundary */
        outmemarea =(unsigned char (* )[2][512]) low_malloc(2*BUFFER_LENGTH, &outmemarea_sel);
        out8buf=(unsigned char (* )[2][512])outmemarea;
        if (((linear_addr(out8buf) % 65536) + BUFFER_LENGTH) > 65536)
          out8buf += BUFFER_LENGTH;

        for (i=0; i<2; i++)
          blockptr[i] = &((*out8buf)[i]);

       /* DMA parameters */
        buffer_addr = linear_addr(out8buf);
        buffer_page = buffer_addr / 65536;
        buffer_ofs  = buffer_addr % 65536;
        for (i=0; i<2; i++)
          {
            block_addr[i] = linear_addr(blockptr[i]);
            block_page[i] = block_addr[i] / 65536;
            block_ofs[i]  = block_addr[i] % 65536;
          }
        memset(out8buf, 0x80, BUFFER_LENGTH * sizeof(unsigned char));

        init_clip8();

      }

    curblock = 0;
    intcount = 0;

    init_volume_table();
    start_dac();
  }

void SB_MixOff(void)
  {
    stop_dac();

    free(volume_table);

    if (!sixteenbit) free(clip_8_buf);

    low_free(outmemarea_sel);
  }

/* Loading and freeing sounds */

static FILE *sound_file;
static long sound_size;

void SB_LoadRAW(FILE * handle,SB_Sample **sound)
{
 static long ssize;
 // Read size
 fread(&ssize,sizeof(long),1,handle);
 /* Allocate sound control structure and sound data block */
  (*sound) = (SB_Sample *) malloc(sizeof(SB_Sample));
  (*sound)->data  = (signed char *)(malloc(ssize));
  (*sound)->size = ssize;
 /* Read sound data and close file (Isn't flat mode nice?) */
 fread((*sound)->data, sizeof(signed char), ssize, handle);
}

void SB_FreeSound(SB_Sample **sound)
  {
    free((*sound)->data);
    free(*sound);
    *sound = NULL;
  }

/* Voice maintainance */

static void deallocate_voice(int voicenum)
  {
    inuse[voicenum] = FALSE;
    voice[voicenum].sound  = NULL;
    voice[voicenum].index  = -1;
    voice[voicenum].volume = 0;
    voice[voicenum].curpos = -1;
    voice[voicenum].loop   = FALSE;
    voice[voicenum].done   = FALSE;
  }

void SB_PlaySound(SB_Sample *sound, int index, unsigned char volume, int loop)
  {
    int i, voicenum;

    voicenum = -1;
    i = 0;

    do
      {
        if (!inuse[i])
          voicenum = i;
        i++;
      }
    while ((voicenum == -1) && (i < VOICES));

    if (voicenum != -1)
      {
        voice[voicenum].sound  = sound;
        voice[voicenum].index  = index;
        voice[voicenum].volume = volume;
        voice[voicenum].curpos = 0;
        voice[voicenum].loop   = loop;
        voice[voicenum].done   = FALSE;

        inuse[voicenum] = TRUE;
        voicecount++;
      }
  }

void SB_StopSound(int index)
  {
    int i;

    for (i=0; i < VOICES; i++)
      if (voice[i].index == index)
        {
          voicecount--;
          deallocate_voice(i);
        }
  }

int  sound_playing(int index)
  {
    int i;

   /* Search for a sound with the specified index */
    for (i=0; i < VOICES; i++)
      if (voice[i].index == index)
        return(TRUE);

   /* Sound not found */
    return(FALSE);
  }

static void update_voices(void)
  {
    int voicenum;

    for (voicenum=0; voicenum < VOICES; voicenum++)
      {
        if (inuse[voicenum])
          {
            if (voice[voicenum].done)
              {
                voicecount--;
                deallocate_voice(voicenum);
              }
          }
      }
  }

/* Mixing */

static void mix_voice(int voicenum)
  {
    SB_Sample *sound;
    int   mixlength;
    signed char *sourceptr;
    signed int *volume_lookup;
    int chunklength;
    int destindex;

   /* Initialization */
    sound = voice[voicenum].sound;

    sourceptr = sound->data + voice[voicenum].curpos;
    destindex = 0;

   /* Compute mix length */
    if (voice[voicenum].loop)
      mixlength = BLOCK_LENGTH;
    else
      mixlength =
       MIN(BLOCK_LENGTH, sound->size - voice[voicenum].curpos);

    volume_lookup =
     (signed int *)(&((*volume_table)[(unsigned char)((((sound_volume/256.0) * voice[voicenum].volume) * (VOLUMES/256.0)))]));

    do
      {
       /* Compute the max consecutive samples that can be mixed */
        chunklength =
         MIN(mixlength, sound->size - voice[voicenum].curpos);

       /* Update the current position */
        voice[voicenum].curpos += chunklength;

       /* Update the remaining samples count */
        mixlength -= chunklength;

       /* Mix samples until end of mixing or end of sound data is reached */
        while (chunklength--)
          mixingblock[destindex++] += volume_lookup[(unsigned char)(*(sourceptr++))];

       /* If we've reached the end of the block, wrap to start of sound */
        if (sourceptr == (sound->data + sound->size))
          {
            if (voice[voicenum].loop)
              {
                voice[voicenum].curpos = 0;
                sourceptr = sound->data;
              }
            else
              {
                voice[voicenum].done = TRUE;
              }
          }
      }
    while (mixlength); /* Wrap around to finish mixing if necessary */
  }

static void silenceblock(void)
  {
    memset(&mixingblock, 0x00, BLOCK_LENGTH*sizeof(signed int));
  }

static void mix_voices(void)
  {
    int i;

    silenceblock();

    for (i=0; i < VOICES; i++)
      if (inuse[i])
        mix_voice(i);
  }






static void uninstall_handler(void)
  {
    _disable();  /* CLI */
    outb(pic_maskport, (inp(pic_maskport) | irq_stopmask));

    _dos_setvect(irq_intvector, oldintvector);

    _enable();   /* STI */

    handler_installed = FALSE;
  }

static void smix_exitproc(void)
  {
    stop_dac();
    SB_Done();
  }

/* лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл */

#endif







#if 0

/* SBDETECT.C                                                         */
/* Copyright 1997 by Ethan Brodsky.  All Rights Reserved.             */
/* 97/6/30 - ebrodsky@pobox.com - http://www.pobox.com/~ebrodsky/     */

#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>

unsigned char read_dsp(int baseio)
  {
    while (!(inp(baseio+0x00E) & 0x80))
      { }

    return(inp(baseio+0x00A));
  }

void write_dsp(int baseio, unsigned char value)
  {
    while ((inp(baseio+0x00C) & 0x80))
      { }

    outb(baseio+0x00C, value);
  }

int reset_dsp(int baseio)
  {
    unsigned i;

    outb(baseio+0x006, 1);
    delay(1);
    outb(baseio+0x006, 0);

    for (i = 65535U; i > 0; i--)
      {
        if ((inp(baseio+0x00E) & 0x80) && (inp(baseio+0x00A) == 0xAA))
          break;
      }

    return(i > 0);
  }

//////////////////////////////////////////////////////////////////////////////

int detect_baseio(void)
  {
   // list of possible addresses, plus "sentinel" -1
    static int val[] = {0x210, 0x220, 0x230, 0x240, 0x250, 0x260, 0x280, -1};
    static int count = sizeof(val)/sizeof(*val) - 1; // don't test -1
    int i;

   // for each possible port
    for (i = 0; i < count; i++)
     // test it by attempting to reset the DSP
      if (reset_dsp(val[i]))
       // if found, break now
        break;

   // base io address, or, if not found, -1, since i=count+1
    return val[i];
  }

//////////////////////////////////////////////////////////////////////////////

void dsp_stopall(int baseio)
  {
   // pause 8/16-bit DMA mode digitized sound I/O
    reset_dsp(baseio);
    write_dsp(baseio, 0xD0);
    write_dsp(baseio, 0xD5);
  }

unsigned char dma_req(void)
 // returns dma_request for all channels (bit7->dma7..bit0->dma0)
  {
    return (inp(0xD0) & 0xF0) | (inp(0x08) >> 4);
  }

int bitcount(unsigned char x)
 // returns number of set bits in byte x
  {
    int i;
    int count = 0;

    for (i = 0; i < 8; i++)
      if (x & (1 << i))
        count++;

    return count;
  }

int bitpos(unsigned char x)
 // returns position lowest set bit in byte x (if none, returns -1)
  {
    int i;

    for (i = 0; i < 8; i++)
      if (x & (1 << i))
        return i;

    return -1;
  }

int find_dma(int baseio, int dmac)
  {
   // dma channels active when sound is not: can't be audio DMA channel
    int dma_maskout = ~0x10;  // initially mask only DMA4 (cascade)

   // dma channels active during audio, minus those masked out
    int dma_mask    = 0;

    int i;

   // stop all dsp activity
    dsp_stopall(baseio);

   // poll to find out which DMA channels are in use without sound
    for (i = 0; i < 100; i++)
      dma_maskout &= ~dma_req();

   // now program card and see what channel becomes active
    if (dmac == 1)
      {
       // 8-bit single-cycle DMA mode digitized sound output
        write_dsp(baseio, 0x14);
        write_dsp(baseio, 0x00);  // lo  one sample
        write_dsp(baseio, 0x00);  // hi
      }
    else
      {
       // 16-bit single-cycle DMA mode digitized sound output
        write_dsp(baseio, 0xB0); // 16-bit, D/A, S/C, FIFO off
        write_dsp(baseio, 0x10); // 16-bit mono signed PCM
        write_dsp(baseio, 0x00); // lo   one sample
        write_dsp(baseio, 0x00); // hi
      }

   // poll to find out which (unmasked) DMA channels are in use with sound
    for (i = 0; i < 100; i++)
      dma_mask |= dma_req() & dma_maskout;

   // stop all dsp activity
    dsp_stopall(baseio);

    if (bitcount(dma_mask) == 1)
      return bitpos(dma_mask);
    else
      return -1;

  }

int detect_dma(int baseio)
  {
    return find_dma(baseio, 1);
  }

int detect_dma16(int baseio)
  {
    return find_dma(baseio, 2);
  }

//////////////////////////////////////////////////////////////////////////////

void dsp_transfer(int baseio, int dma8)
  {
    static int dma_pageports[] = {0x87, 0x83, 0x81, 0x82, -1, 0x8B, 0x89, 0x8A};

    int dma_maskport   = 0x0A;
    int dma_modeport   = 0x0B;
    int dma_clrptrport = 0x0C;
    int dma_addrport   = 0x00+2*dma8;
    int dma_countport  = 0x01+2*dma8;
    int dma_pageport   = dma_pageports[dma8];

   // mask DMA channel
    outb(dma_maskport,   0x04 | dma8);

   // write DMA mode: single-cycle read transfer
    outb(dma_modeport,   0x48 | dma8);

   // clear byte-pointer flip-flop
    outb(dma_clrptrport, 0x00);

   // one transfer
    outb(dma_countport,  0x00); // lo
    outb(dma_countport,  0x00); // hi

   // address ????????
    outb(dma_addrport,   0); // lo
    outb(dma_addrport,   0); // hi
    outb(dma_pageport,   0);

   // unmask DMA channel
    outb(dma_maskport,   0x00 | dma8);

   // 8-bit single-cycle DMA mode digitized sound outbut
    write_dsp(baseio, 0x14);
    write_dsp(baseio, 0x00);    // lo  one sample
    write_dsp(baseio, 0x00);    // hi
  }

static void ((interrupt far *old_handler[16])(void));

static int irq_hit[16];
static int irq_mask[16];

void clear_irq_hit(void)
  {
    int i;

    for (i = 0; i < 16; i++)
      irq_hit[i] = 0;
  }

void interrupt irq2_handler(void)  { irq_hit[2]  = 1;  _chain_intr(old_handler[2]);  }
void interrupt irq3_handler(void)  { irq_hit[3]  = 1;  _chain_intr(old_handler[3]);  }
void interrupt irq5_handler(void)  { irq_hit[5]  = 1;  _chain_intr(old_handler[5]);  }
void interrupt irq7_handler(void)  { irq_hit[7]  = 1;  _chain_intr(old_handler[7]);  }
void interrupt irq10_handler(void) { irq_hit[10] = 1;  _chain_intr(old_handler[10]); }

int detect_irq(int baseio, int dma8)
  {
    int pic1_oldmask, pic2_oldmask;

    int irq = -1;
    int i;

   // install handlers
    old_handler[2]  = _dos_getvect(0x0A);  _dos_setvect(0x0A, irq2_handler);
    old_handler[3]  = _dos_getvect(0x0B);  _dos_setvect(0x0B, irq3_handler);
    old_handler[5]  = _dos_getvect(0x0D);  _dos_setvect(0x0D, irq5_handler);
    old_handler[7]  = _dos_getvect(0x0F);  _dos_setvect(0x0F, irq7_handler);
    old_handler[10] = _dos_getvect(0x72);  _dos_setvect(0x72, irq10_handler);

   // save old IRQ mask and unmask IRQs
    pic1_oldmask = inp(0x21);   outb(0x21, pic1_oldmask & 0x53);
    pic2_oldmask = inp(0xA1);   outb(0xA1, pic2_oldmask & 0xFB);

    clear_irq_hit();

   // wait to see what interrupts are triggered without sound
    delay(100);

   // mask out any interrupts triggered without sound
    for (i = 0; i < 16; i++)
      irq_mask[i] = irq_hit[i];

    clear_irq_hit();

   // try to trigger an interrupt using DSP command F2
    write_dsp(baseio, 0xF2);

    delay(10);

   // detect any interrupts triggered
    for (i = 0; i < 16; i++)
      if (irq_hit[i] && !irq_mask[i])
        irq = i;

   // if F2 fails to trigger an interrupt, run a short transfer
    if (irq == -1)
      {
        reset_dsp(baseio);
        dsp_transfer(baseio, dma8);

        delay(10);

       // detect any interrupts triggered
        for (i = 0; i < 16; i++)
          if (irq_hit[i] && !irq_mask[i])
            irq = i;
      }

   // reset DSP in case we've confused it
    reset_dsp(baseio);

   // remask IRQs
    outb(0x21, pic1_oldmask);
    outb(0xA1, pic2_oldmask);

   // uninstall handlers
    _dos_setvect(0x0A, old_handler[2]);
    _dos_setvect(0x0B, old_handler[3]);
    _dos_setvect(0x0D, old_handler[5]);
    _dos_setvect(0x0F, old_handler[7]);
    _dos_setvect(0x72, old_handler[10]);

    return irq;
  }

//////////////////////////////////////////////////////////////////////////////

int sb_detect(int *baseio, int *irq, int *dma, int *dma16)
  {
    *irq   = -1;
    *dma16 = -1;

    if ((*baseio = detect_baseio()) == -1)
      return 0;

    if ((*dma = detect_dma(*baseio)) == -1)
      return 0;

    if ((*dma16 = detect_dma16(*baseio)) == -1)
      return 0;

    if ((*irq = detect_irq(*baseio, *dma)) == -1)
      return 0;

    return 1;
  }

int main()
  {
    int baseio, irq, dma, dma16;
    int detected;

    detected = sb_detect(&baseio, &irq, &dma, &dma16);

    printf("%s \n", detected ? "Success" : "Failure");

    printf("baseio: %Xh \n", baseio);
    printf("irq:    %i  \n", irq);
    printf("dma:    %i  \n", dma);
    printf("dma16:  %i  \n", dma16);

    return !detected;
  }

#endif

