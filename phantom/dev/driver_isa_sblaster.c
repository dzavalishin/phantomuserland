#if 0
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


#define DEBUG_MSG_PREFIX "sblaster"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/drivers.h>

#include <i386/pio.h>
#include <phantom_libc.h>

#include <hal.h>

#include <x86/comreg.h>


typedef struct
{
    int dma;
    int dma16;

    int autoinit;
    int sixteenbit;

    //! interrupts handled - TODO move to global dev code?
    int intcount;

    int curblock;
} sb_t;




static int
sb_detect(int unit, int addr)
{

}

static int sb_start(phantom_device_t *dev); // Start device (begin using)
static int sb_stop(phantom_device_t *dev);  // Stop device

// Access from kernel - can block!
static int sb_read(struct phantom_device *dev, void *buf, int len);
static int sb_write(struct phantom_device *dev, const void *buf, int len);


static void sb_interrupt( void *_dev );

// TODO this is to be moved to driver_map.c and be kept in driver tables
static int seq_number = 0;

phantom_device_t * driver_isa_sb_probe( int port, int irq, int stage )
{
    if( !sb_detect( seq_number, port) )
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

    sb->baudRate = 9600;
    sb->stopBits = 1;
    sb->dataBits = 8;
    sb->parity = 0;

    return dev;
}




static void sb_interrupt( void *_dev )
{
    phantom_device_t * dev = _dev;

    int unit = dev->seq_number;
    int addr = dev->iobase;

    sb_t *sb = (sb_t *)drv_private;

    SHOW_FLOW( 9, "sound blaster %d interrupt", unit );

    sb->intcount++;

    if(!sb->autoinit)   /* Start next block quickly if not using auto-init DMA */
    {
        startblock_sc();
        copy_sound();
        curblock = !curblock;  /* Toggle block */
    }

    update_voices();
    mix_voices();

    if(sb->autoinit)
    {
        copy_sound();
        sb->curblock = !sb->curblock;  /* Toggle block */
      }

}





// Start device (begin using)
static int sb_start(phantom_device_t *dev)
{
    int addr = dev->iobase;
    int unit = dev->seq_number;

    sb_t *sb = (sb_t *)drv_private;

    //take_dev_irq(dev);
    SHOW_INFO( 1, "start sblaster port = %x, unit %d", addr, unit);


    return 0;
}

// Stop device
static int sb_stop(phantom_device_t *dev)
{
    int addr = dev->iobase;
    sb_t *sb = (sb_t *)drv_private;

    return 0;
}




static int sb_read(struct phantom_device *dev, void *buf, int len)
{
    return -EIO;
}

static int sb_write(struct phantom_device *dev, const void *buf, int len)
{
    return -EIO;
}








static void SB_WriteDSP(phantom_device_t *dev, BYTE value)
  {
    while ((inp(writeport) & 0x80));
    outb(writeport, value);
  }

static BYTE SB_ReadDSP(phantom_device_t *dev)
  {
    while (!(inp(pollport) & 0x80));
    return(inp(readport));
  }

static int SB_ResetDSP(phantom_device_t *dev)
  {
    int i;

    outb(resetport, 1);
    for (i=0; i < 100; i++)    /* The delay function doesn't work correctly */
      { };
    outb(resetport, 0);

    i = 100;
    while ((i-- > 0) && (SB_ReadDSP() != 0xAA));

    return(i > 0);
  }



//! Starts a single-cycle DMA transfer
static void startblock_sc(phantom_device_t *dev)     
{
    outb(dma_maskport,   dma_stopmask);
    outb(dma_clrptrport, 0x00);
    outb(dma_modeport,   dma_mode);
    outb(dma_addrport,   lo(block_ofs[curblock]));
    outb(dma_addrport,   hi(block_ofs[curblock]));
    outb(dma_countport,  lo(BLOCK_LENGTH-1));
    outb(dma_countport,  hi(BLOCK_LENGTH-1));
    outb(dma_pageport,   block_page[curblock]);
    outb(dma_maskport,   dma_startmask);

    SB_WriteDSP(dev, 0x14);                /* 8-bit single-cycle DMA sound output  */
    SB_WriteDSP(dev, lo(BLOCK_LENGTH-1));
    SB_WriteDSP(dev, hi(BLOCK_LENGTH-1));
}





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


#endif


// 8-chan mixing lib

#define TRUE  1
#define FALSE 0
#define ON  1
#define OFF 0

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


#define BYTE unsigned char


#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) > (b)) ? (b) : (a))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) > (b)) ? (b) : (a))


static int pic_rotateport;
static int pic_maskport;


static char irq_startmask;
static char irq_stopmask;
static char irq_intvector;


static void (interrupt far *oldintvector)(void);

static int handler_installed;

void SB_LoadWav(FILE * handle, SB_Sample **sound)
{

}



void install_handler(void);
void uninstall_handler(void);
void smix_exitproc(void);


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


//static int handler_installed;

static unsigned char sound_volume;

/* 8-bit clipping */

static unsigned char (*clip_8_buf)[256*VOICES];
static unsigned char (*clip_8)[256*VOICES];



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

static void copy_sound16(void)
  {
    int i;
    signed short *destptr;

    destptr = (short * __far)blockptr[curblock];

    for (i=0; i < BLOCK_LENGTH; i++)
      destptr[i] = mixingblock[i];
  }

static void copy_sound8(void)
  {
    int i;
    unsigned char *destptr;

    destptr   = (unsigned char * __far)blockptr[curblock];

    for (i=0; i < BLOCK_LENGTH; i++)
      destptr[i] = (*clip_8)[mixingblock[i] >> 5];
  }

static void copy_sound(void)
  {
    if (sixteenbit)
      copy_sound16();
    else
      copy_sound8();
  }






/* лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл */

#endif









#endif
