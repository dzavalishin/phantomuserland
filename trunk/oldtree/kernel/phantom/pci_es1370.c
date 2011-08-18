#if HAVE_PCI
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ES1370 sound card driver.
 *
**/
#define DEV_NAME "es1370"
#define DEBUG_MSG_PREFIX "es1370"
#include <debug_ext.h>
#define debug_level_flow 9
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/drivers.h>
#include <sys/libkern.h>

#include <ia32/pio.h>
#include <errno.h>
#include <assert.h>
#include <hal.h>
#include <time.h>
#include <sys/ioctl.h>

#include <dev/pci/es1370.h>

#define u32 u_int32_t
#define u16 u_int16_t
#define u8 u_int8_t
//#define esSleep(__us) do { long us = __us; do { tenmicrosec(); us -= 100; } while( __us > 0); } while(0)
#define esSleep(__us) do { long ms = __us/1000; if(ms == 0) ms = 1; hal_sleep_msec(ms);  } while(0)

static errno_t check_es1370_sanity(int iobase);
static errno_t init_es1370(phantom_device_t *dev);
static void reset_codec(phantom_device_t *dev);

static void set_sampling_rate(phantom_device_t *dev, u_int32_t rate);

static void es1370_interrupt(void *arg);
static void w_dpc_func(void *);

static void start_dac(phantom_device_t *dev);


static void readSamplesFromRecordBuffer(phantom_device_t *dev);
static void writeSamplesToPlaybackBuffer(phantom_device_t *dev);



static int es1370_start(phantom_device_t *dev);
static int es1370_stop(phantom_device_t *dev);

static int es1370_write(phantom_device_t *dev, const void *buf, int len);
static int es1370_read(phantom_device_t *dev, void *buf, int len);

static int es1370_ioctl(struct phantom_device *dev, int type, void *buf, int len);

static errno_t	es1370_getproperty( struct phantom_device *dev, const char *pName, char *pValue, int vlen );
static errno_t	es1370_setproperty( struct phantom_device *dev, const char *pName, const char *pValue );
static errno_t	es1370_listproperties( struct phantom_device *dev, int nProperty, char *pValue, int vlen );



static int seq_number = 0;
phantom_device_t * driver_es1370_probe( pci_cfg_t *pci, int stage )
{

    (void) stage;

    SHOW_FLOW( 1, "Probe for " DEV_NAME " stage %d", stage );

    phantom_device_t * dev = malloc(sizeof(phantom_device_t));

    int i;
    for (i = 0; i < 6; i++)
    {
        if (pci->base[i] > 0xffff)
        {
            dev->iomem = (pci->base[i]);
            dev->iomemsize = pci->size[i];
            SHOW_INFO( 1, "mem base 0x%lx, size 0x%lx", dev->iomem, dev->iomemsize);
        } else if( pci->base[i] > 0) {
            dev->iobase = pci->base[i];
            SHOW_INFO( 1, "io_port 0x%x", dev->iobase);
        }
    }

    dev->irq = pci->interrupt;

    // Gets port 0. uninited by BIOS? Need explicit PCI io addr assign?
    if( dev->iobase == 0 )
    {
        SHOW_ERROR0( 0, "No io port?" );
        goto free;
    }

    SHOW_FLOW( 1, "Look for " DEV_NAME " at io %X", dev->iobase );
    if( check_es1370_sanity(dev->iobase) )
        goto free;

    dev->name = DEV_NAME;
    dev->seq_number = seq_number++;

    dev->dops.start = es1370_start;
    dev->dops.stop  = es1370_stop;
    dev->dops.read  = es1370_read;
    dev->dops.write = es1370_write;
    dev->dops.ioctl = es1370_ioctl;

    dev->dops.getproperty = es1370_getproperty;
    dev->dops.setproperty = es1370_setproperty;
    dev->dops.listproperties = es1370_listproperties;

    if( hal_irq_alloc( dev->irq, &es1370_interrupt, dev, HAL_IRQ_SHAREABLE ) )
    {
        SHOW_ERROR( 0, "IRQ %d is busy", dev->irq );
        goto free;
    }

    es1370_t *es = calloc(1,sizeof(es1370_t));
    assert(es);

#if ES1370_CBUF
    hal_spin_init( &es->w_lock );
    dpc_request_init( &es->w_dpc, w_dpc_func );
#endif

    dev->drv_private = es;

    if( init_es1370(dev) )
        goto free1;



    return dev;
free1:
    free(es);

free:
    free(dev);
    return 0;
}




static errno_t check_es1370_sanity(int iobase)
{
    int is = inl(iobase+ES1370_IRQ_STATUS);

    SHOW_FLOW( 1, "irq status 0x%x", is );
    is &= ~0x800007FF;
    if( is )  return ENXIO;

    return 0;
}


static errno_t init_es1370(phantom_device_t *dev)
{
    es1370_t *es = dev->drv_private;

    u_int32_t ctl = 0;
    ctl |= ControlAdcStop|ControlSerrDis|ControlCdcEn;
    ctl |= (3<<ControlWtsrselShift); // 44K clock

    outl(dev->iobase+ES1370_CONTROL, ctl);

    outl(dev->iobase+ES1370_SERIAL_CONTROL, 0);

    hal_sem_init( &es->w_sem, DEV_NAME );

    es->dac_active = 0;
    es->adc_active = 0;

    es->silence = 0;

    es->samplerate = 44100;
    es->nchannels = 2;
    es->nbits = 16;
    //es->adc_bitsPerSample = 16;
    //es->dac_bitsPerSample = 16;

    es->oddDac2 = 0;
    es->oddAdc = 0;

    es->writtenSamples = 0;

    int size_bytes = ES1370_BUFSIZE;

    // TODO free
    hal_pv_alloc( &es->dac2BufferPa, &es->dac2Buffer, size_bytes );
    es->dac2BufferSize = size_bytes;

    hal_pv_alloc( &es->adcBufferPa, &es->adcBuffer, size_bytes );
    es->adcBufferSize = size_bytes;

    reset_codec(dev);
    //start_dac(dev);
    //set_sampling_rate(u_int32_t rate)
    return 0;
}










static void setPlayBuffer(phantom_device_t *dev, physaddr_t address, size_t size)
{
    outl(dev->iobase + MemoryPageRegister, DacFrameInformation);
    //outl(dev->iobase + Dac2PciAddressRegister, address & ~0xc0000000 /* physical address */);
    outl(dev->iobase + Dac2PciAddressRegister, address /* physical address */);
    outl(dev->iobase + MemoryPageRegister, DacFrameInformation);
    outl(dev->iobase + Dac2BufferSizeRegister, (size >> 2) - 1); // set the number of longwords in a buffer minus one.
}

static void setRecordBuffer(phantom_device_t *dev, physaddr_t address, size_t size)
{
    outl(dev->iobase + MemoryPageRegister, AdcFrameInformation);
    //outl(dev->iobase + AdcPciAddressRegister, address & ~0xc0000000 /* physical address */);
    outl(dev->iobase + AdcPciAddressRegister, address /* physical address */);
    outl(dev->iobase + MemoryPageRegister, AdcFrameInformation);
    outl(dev->iobase + AdcBufferSizeRegister, (size >> 2) - 1); // set the number of longwords in a buffer minus one.
}

static void setPlaybackSampleCount(phantom_device_t *dev, u32 count)
{
    outl(dev->iobase + Dac2SampleCountRegister, count-1);
}

static void setRecordSampleCount(phantom_device_t *dev, u32 count)
{
    outl(dev->iobase + AdcSampleCountRegister, count-1);
}

static void setSamplingRate(phantom_device_t *dev, u32 rate)
{
    u32 control = inl(dev->iobase + ES1370_CONTROL);
    control &= ~ControlPclkdiv;
    control |= (1411200/rate-2) << ControlPclkdivShift;
    outl(dev->iobase + ES1370_CONTROL, control);
}

static void setPlaybackFormat(phantom_device_t *dev, u8 channels, u8 bits)
{
    assert(bits == 8 || bits == 16);
    assert(channels == 1 || channels == 2);

    u32 control = inl(dev->iobase + ES1370_SERIAL_CONTROL);
    control &= ~(SerialP2smb | SerialP2seb);
    if (channels == 2)
    {
        control |= SerialP2smb;
    }
    if (bits == 16)
    {
        control |= SerialP2seb;
    }
    outl(dev->iobase + ES1370_SERIAL_CONTROL, control);
}

static void setRecordFormat(phantom_device_t *dev, u8 channels, u8 bits)
{
    assert(bits == 8 || bits == 16);
    assert(channels == 1 || channels == 2);
    u32 control = inl(dev->iobase + ES1370_SERIAL_CONTROL);

    control &= ~(SerialR1smb | SerialR1seb);
    if (channels == 2)
    {
        control |= SerialR1smb;
    }
    if (bits == 16)
    {
        control |= SerialR1seb;
    }
    outl(dev->iobase + ES1370_SERIAL_CONTROL, control);
}

















static void play(phantom_device_t *dev)
{
    es1370_t *es = dev->drv_private;
    assert(es->dac_active);
    //u8 bits = lineDac2->getBitsPerSample();
    u8 bits = es->nbits;

    u32 control = inl(dev->iobase + ES1370_SERIAL_CONTROL);
    control &= ~(SerialP2endinc | SerialP2stinc | SerialP2loopsel | SerialP2pause | SerialP2dacsen);
    control |= SerialP2inten | ((bits/8) << SerialP2endincShift);
    outl(dev->iobase + ES1370_SERIAL_CONTROL, control);

    control = inl(dev->iobase + ES1370_CONTROL);
    control |= ControlDac2En;
    outl(dev->iobase + ES1370_CONTROL, control); // start DMA
}

static void record(phantom_device_t *dev)
{
    u32 control = inl(dev->iobase + ES1370_SERIAL_CONTROL);
    control &= ~SerialR1loopsel;
    control |= SerialR1inten;
    outl(dev->iobase + ES1370_SERIAL_CONTROL, control);

    control = inl(dev->iobase + ES1370_CONTROL);
    control |= ControlAdcEn;
    outl(dev->iobase + ES1370_CONTROL, control); // start DMA
}


























static void start_dac(phantom_device_t *dev)
{
    es1370_t *es = dev->drv_private;

    if(es->dac_active)
        return;
    SHOW_FLOW0( 10, "dac" );

    //u8 channels = line->getChannels();
    //u16 rate = line->getSamplingRate();
    //u8 bits = line->getBitsPerSample();
    //u8 silence = (bits == 8) ? 128 : 0;

    //u16 rate = 44100;
    u8 bits = es->nbits;
    u8 channels = es->nchannels;

    memset(es->dac2Buffer, es->silence, es->dac2BufferSize);

    setPlayBuffer(dev, es->dac2BufferPa, es->dac2BufferSize);
    setPlaybackSampleCount(dev, es->dac2BufferSize / 2 / channels / (bits / 8));
    setSamplingRate(dev, es->samplerate);
    setPlaybackFormat(dev, channels, bits);

    es->dac_active = 1;

    play(dev);
    // Fill the first buffer now or else it won't be double buffer
    writeSamplesToPlaybackBuffer(dev);

}

static void start_adc(phantom_device_t *dev) __attribute__((used));

static void start_adc(phantom_device_t *dev)
{
    es1370_t *es = dev->drv_private;

    if(es->adc_active)
        return;

    //u8 channels = line->getChannels();
    //u16 rate = line->getSamplingRate();
    //u8 bits = line->getBitsPerSample();
    //u8 silence = (bits == 8) ? 128 : 0;

    //u16 rate = 44100;
    u8 bits = es->nbits;
    u8 channels = es->nchannels;
    //u8 silence = 0;
    memset(es->adcBuffer, es->silence, es->dac2BufferSize);

    setRecordBuffer(dev, es->adcBufferPa, es->adcBufferSize);
    setRecordSampleCount(dev, es->adcBufferSize / 2 / channels / (bits / 8));
    setSamplingRate(dev, es->samplerate);
    setRecordFormat(dev, channels, bits);

    record(dev);
}




static void stop_dac(phantom_device_t *dev)
{
    es1370_t *es = dev->drv_private;

    // stop playback.
    u32 control = inl(dev->iobase + ES1370_SERIAL_CONTROL);
    control |= SerialP2pause;
    outl(dev->iobase + ES1370_SERIAL_CONTROL, control);

    control = inl(dev->iobase + ES1370_CONTROL);
    control &= ~ControlDac2En;
    outl(dev->iobase + ES1370_CONTROL, control);
    es->dac_active = 0;
}

static void stop_adc(phantom_device_t *dev)
{
    es1370_t *es = dev->drv_private;

    // stop recording.
    u32 control = inl(dev->iobase + ES1370_CONTROL);
    control &= ~ControlAdcEn;
    outl(dev->iobase + ES1370_CONTROL, control);
    es->adc_active = 0;
}

#if 1

#if ES1370_CBUF

// get from userland

static size_t read_play_stream(phantom_device_t *dev, void *ptr, int len )
{
    es1370_t *es = dev->drv_private;
    int res = 0;

    SHOW_FLOW( 1, "req %d", len );

    int ie = hal_save_cli();
    hal_spin_lock( &es->w_lock );

    if(es->w_cbuf)
    {
        //size_t src_len = cbuf_get_len(es->w_cbuf) - es->w_read_pos;
        size_t src_len = es->w_write_pos - es->w_read_pos;
        int nc = imin(len, src_len );

        if( nc > 0 )
        {
            assert(! cbuf_memcpy_from_chain(ptr, es->w_cbuf, es->w_read_pos, nc) );
            len -= nc;
            ptr += nc;
            res += nc;
            es->w_read_pos += nc;
            hal_sem_release( &es->w_sem );
        }
    }

    hal_spin_unlock( &es->w_lock );
    if( ie ) hal_sti();

    SHOW_FLOW( 1, "done %d", res );

    return res;
}

static void trunc_cbuf(phantom_device_t *dev)
{
    es1370_t *es = dev->drv_private;

    if(!es->w_cbuf)
        return;

    int ie = hal_save_cli();
    hal_spin_lock( &es->w_lock );

    //size_t clen = cbuf_get_len(es->w_cbuf);

    if( es->w_read_pos > 0 )
    {
        SHOW_FLOW( 1, "before rpos %d wpos %d", es->w_read_pos, es->w_write_pos );
        //cbuf *tcb = cbuf_truncate_head( es->w_cbuf, es->w_read_pos, 1);

        es->w_cbuf = cbuf_truncate_head( es->w_cbuf, es->w_read_pos, 1);
        es->w_write_pos -= es->w_read_pos;
        es->w_read_pos = 0;
        SHOW_FLOW( 1, "after rpos %d wpos %d", es->w_read_pos, es->w_write_pos );
    }

    hal_spin_unlock( &es->w_lock );
    if( ie ) hal_sti();
}

#define MAX_CBUF (32*4096)

static size_t write_play_stream(phantom_device_t *dev, const void *ptr, int len )
{
    es1370_t *es = dev->drv_private;

    SHOW_FLOW( 1, "req %d", len );

    int ie = hal_save_cli();
    hal_spin_lock( &es->w_lock );

    int nc, ret = 0;

    int maxleft = MAX_CBUF - es->w_write_pos;
    nc = imin( maxleft, len );

    assert( nc >= 0 );
    SHOW_FLOW( 1, "nc %d", nc );

    if( nc > 0 )
    {
        if(!es->w_cbuf)
        {
            assert(es->w_write_pos == 0);
            assert(es->w_read_pos == 0);
            es->w_cbuf = cbuf_get_chain(nc);
        }
        else
        {
            assert(!cbuf_extend_tail(es->w_cbuf, nc));
        }

        assert(!cbuf_memcpy_to_chain( es->w_cbuf, es->w_write_pos, ptr, nc));
    }

    ret += nc;
    es->w_write_pos += nc;

    hal_sem_zero( &es->w_sem );

    hal_spin_unlock( &es->w_lock );
    if( ie ) hal_sti();

    trunc_cbuf(dev);

    if( len > ret )
        hal_sem_acquire( &es->w_sem );

    SHOW_FLOW( 1, "done %d", ret );

    return ret;
}

#else // ES1370_CBUF

static size_t read_play_stream(phantom_device_t *dev, void *ptr, int len )
{
    es1370_t *es = dev->drv_private;
    int res = 0;

    while( (len > 0) && (es->w_len > 0) )
    {
        int nc = imin(len, es->w_len );
        memcpy( ptr, es->w_buf, nc );
        len -= nc;
        ptr += nc;
        res += nc;
        es->w_buf += nc;
        es->w_len -= nc;
    }

    if( es->w_len <= 0 )
        hal_sem_release( &es->w_sem );

    return res;
}

#endif // ES1370_CBUF



#else
// generate itself

#if 1
static const char  intro[] = {
#include "intro.ci"
};

static void sw_memcpy( char *o, const char *i, size_t nc )
{
    char c1, c2;

    while( nc > 1 )
    {
        c1 = *i++;
        c2 = *i++;
        *o++ = c2;
        *o++ = c1;
        nc -= 2;
    }
}


static size_t read_play_stream(phantom_device_t *dev, void *ptr, int len )
{
    static const char *ip = intro;
    static int         is = sizeof(intro);

    int res = 0;

    //if( is < 4 ) return 0;

    while( (len > 3) && (is > 3) )
    {
        int nc = imin(len, is ) & ~3; // 4 bytes min
        sw_memcpy( ptr, ip, nc );
        len -= nc;
        ptr += nc;
        res += nc;

        ip += nc;
        is -= nc;
    }

    return res;
}

#else


static size_t read_play_stream(phantom_device_t *dev, void *ptr, int len )
{
    (void) dev;
    char meander[] = "adgjlorzzroljgda                "; // really poor man's sine ;)
    int res = 0;

    while(len > 0)
    {
        int nc = imin(len, sizeof( meander) );
        memcpy( ptr, meander, nc );
        len -= nc;
        ptr += nc;
        res += nc;
    }

    return res;
}
#endif
#endif




static void writeSamplesToPlaybackBuffer(phantom_device_t *dev)
{
    es1370_t *es = dev->drv_private;

    u8* ptr = es->dac2Buffer;
    if(es->oddDac2)
        ptr += es->dac2BufferSize / 2;

    unsigned int count = read_play_stream(dev, ptr, es->dac2BufferSize / 2);
#if 0
    if (count == 0)
    {
        stop_dac(dev);
    }
    else
#endif
    {
        if (count < es->dac2BufferSize / 2)
            memset(ptr + count, es->silence, (es->dac2BufferSize / 2) - count);
        es->oddDac2 = !es->oddDac2; // select the other buffer.

        es->writtenSamples += es->dac2BufferSize / 2;
    }
}

static void readSamplesFromRecordBuffer(phantom_device_t *dev)
{
    es1370_t *es = dev->drv_private;

    u8* ptr = es->adcBuffer;
    if(es->oddAdc)
    {
        ptr += es->adcBufferSize / 2;
    }

    //unsigned int count = lineAdc->write(ptr, es->adcBufferSize / 2);
    unsigned int count = 1;
    if (count == 0)
    {
        stop_adc(dev);
    }
    else
    {
        //u8 bits = es->nbits;//es->adc_bitsPerSample;
        if (count < es->adcBufferSize / 2)
        {
            //u8 silence = (bits == 8) ? 128 : 0;
            memset(ptr + count, es->silence, es->adcBufferSize / 2 - count);
        }
        es->oddAdc = !es->oddAdc; // select the other buffer.
    }
}



#if ES1370_CBUF
static void w_dpc_func(void *arg)
{
    phantom_device_t *dev = arg;

    writeSamplesToPlaybackBuffer(dev);
    u_int32_t control = inl(dev->iobase + ES1370_SERIAL_CONTROL);
    control |= SerialP2inten;
    outl(dev->iobase + ES1370_SERIAL_CONTROL, control);
}
#endif

static void es1370_interrupt(void *arg)
{
    phantom_device_t *dev = arg;
    es1370_t *es = dev->drv_private;

    /*
    switch (irq)
    {
    case 0:        start(&inputLine);        return 0;
    case 1:        start(&outputLine);        return 0;
    default:        break;
    }
    */
//start_dac(dev);
//start_adc(dev);

    u_int32_t status = inl(dev->iobase + ES1370_IRQ_STATUS);
    SHOW_FLOW( 10, "%s %b VC %d", (status&0x8000)?"IRQ":"   ",status, "\020\1ADC\2DAC2\3DAC1\4UART\5MCCB\11Cwrip\12Cbusy\13Cstat", (status &StatusVc)>>StatusVcShift );
    if ((status & StatusIntr) == 0)
        return;

    if(status & StatusDac2)
    {
        u_int32_t control = inl(dev->iobase + ES1370_SERIAL_CONTROL);
        control &= ~SerialP2inten;
        outl(dev->iobase + ES1370_SERIAL_CONTROL, control);
        if(es->dac_active)
        {
#if ES1370_CBUF
            dpc_request_trigger( &es->w_dpc, dev);

#else
            writeSamplesToPlaybackBuffer(dev);
            control |= SerialP2inten;
            outl(dev->iobase + ES1370_SERIAL_CONTROL, control);
#endif
        }
#if 0
        // Attempt to refill buffers if underflow is detected.
        // Helps somehow, but not completely.

        int maxRefill = 2;
        while(maxRefill-- > 0)
        {
            unsigned int nSamples = inl( dev->iobase+Dac2SampleCountRegister );
            unsigned nBytes = (nSamples >> 16) * 4;

            //SHOW_FLOW( 1, "?refill %d-%d", 4*(nSamples & 0xFFFF), nBytes );
            if( nBytes < 2048 )
            {
                SHOW_FLOW( 1, "refill %d", nBytes );
                writeSamplesToPlaybackBuffer(dev);
            }
            else
                break;
        }
        /*
        SHOW_FLOW( 1, "?refill %d-%d", es->writtenSamples, nSamples );
        if( es->writtenSamples - nSamples < es->dac2BufferSize/2 )
        {
            SHOW_FLOW( 1, "refill %d-%d", es->writtenSamples, nSamples );
            writeSamplesToPlaybackBuffer(dev);
        }
        */
#endif
    }

    if(status & StatusAdc)
    {
        u_int32_t control = inl(dev->iobase + ES1370_SERIAL_CONTROL);
        control &= ~SerialR1inten;
        outl(dev->iobase + ES1370_SERIAL_CONTROL, control);
        if(es->adc_active)
        {
            readSamplesFromRecordBuffer(dev);

            control |= SerialR1inten;
            outl(dev->iobase + ES1370_SERIAL_CONTROL, control);
        }
    }

    // just clear flags.
    if(status & StatusDac1)
    {
        u_int32_t control = inl(dev->iobase + ES1370_SERIAL_CONTROL);
        control &= ~SerialP1inten;
        outl(dev->iobase + ES1370_SERIAL_CONTROL, control);
    }

    if(status & StatusMccb)
    {
        u_int32_t control = inl(dev->iobase + ES1370_CONTROL);
        control &= ~ControlCcbIntrm;
        outl(dev->iobase + ES1370_CONTROL, control);
    }

}






static void set_sampling_rate(phantom_device_t *dev, u_int32_t rate)
{
    u_int32_t control = inl(dev->iobase + ES1370_CONTROL);
    control &= ~ControlPclkdiv;
    control |= (1411200/rate-2) << ControlPclkdivShift;
    outl(dev->iobase + ES1370_CONTROL, control);
}






#define setCodec(a,b) _setCodec(dev,a,b)

static int _setCodec(phantom_device_t *dev, u8 codecControlRegister, u8 value)
{
    int retry = 10;
    int ret;
    while ((ret = (inl(dev->iobase + ES1370_IRQ_STATUS) & StatusCstat)) && retry--)
    {
        esSleep(10000); // 1msec
    }

    if (ret == 0)
    {
        outw(dev->iobase + CodecWriteRegister, ((u16) codecControlRegister << 8) | value); // This register must be accessed as a word.
        //codec[codecControlRegister] = value; // save
        esSleep(1000); // 100usec
        return 0;
    }

    SHOW_ERROR0( 0, "Timeout"); // wrong device?
    return -1;
}

static void reset_codec(phantom_device_t *dev)
{
    es1370_t *es = dev->drv_private;

    es->volume = DefaultGainLevel;

    setCodec(CodecResPd, RespdNormalOperation);
    setCodec(CodecCsel, 0); // set the clocks for codec.
    esSleep(200000); // 20msec
    setCodec(CodecAdsel, 0); // select input mixer as the input source to ADC.

    // volumes.
    setCodec(CodecVolVoiceL, DefaultGainLevel);
    setCodec(CodecVolVoiceR, DefaultGainLevel);

    setCodec(CodecVolFmL, VolMute | DefaultGainLevel);
    setCodec(CodecVolFmR, VolMute | DefaultGainLevel);

    setCodec(CodecVolCdL, VolMute | DefaultGainLevel);
    setCodec(CodecVolCdR, VolMute | DefaultGainLevel);

    setCodec(CodecVolLineL, DefaultGainLevel);
    setCodec(CodecVolLineR, DefaultGainLevel);

    setCodec(CodecVolAuxL, VolMute | DefaultGainLevel);
    setCodec(CodecVolAuxR, VolMute | DefaultGainLevel);

    setCodec(CodecVolMono1, VolMute | DefaultGainLevel);
    setCodec(CodecVolMono2, VolMute | DefaultGainLevel);

    setCodec(CodecVolMic, DefaultGainLevel);
    setCodec(CodecVolMonoOut, 0); // 0dB.

    // mic
    setCodec(CodecMgain, MgainAmpOn);

    // mixers.
    setCodec(CodecOmix1, 0);
    setCodec(CodecOmix2, Omix2VoiceL | Omix2VoiceR);

    setCodec(CodecLimix1, Imix1LineL | Imix1Mic);
    setCodec(CodecRimix1, Imix1LineR | Imix1Mic);

    setCodec(CodecLimix2, MixerAllOff);
    setCodec(CodecRimix2, MixerAllOff);

    // turn on the master volume.
    setCodec(CodecVolMasterL, DefaultAttLevel);
    setCodec(CodecVolMasterR, DefaultAttLevel);
}

























static int es1370_start(phantom_device_t *dev)
{
    es1370_t *es = dev->drv_private;

    SHOW_FLOW( 1, "dev %p", dev );
    start_dac(dev);

    setCodec(CodecVolMasterL, es->volume);
    setCodec(CodecVolMasterR, es->volume);

    return 0;
}

static int es1370_stop(phantom_device_t *dev)
{
    //es1370_t *es = dev->drv_private;

    SHOW_FLOW( 1, "dev %p", dev );

    setCodec(CodecVolMasterL, 0 );
    setCodec(CodecVolMasterR, 0 );

    stop_dac(dev);

#if ES1370_CBUF
    trunc_cbuf(dev);
#endif

    return 0;
}



static int es1370_read(phantom_device_t *dev, void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;

    return -1;
}

static int es1370_write(phantom_device_t *dev, const void *buf, int len)
{
#if ES1370_CBUF
    int ret = 0;

    while( len > 0 )
    {
        size_t nc = write_play_stream( dev, buf, len );
        buf += nc;
        len -= nc;
    }

    return ret;
#else

    es1370_t *es = dev->drv_private;

    //SHOW_FLOW( 1, "dev %p buf %p len %d", dev, buf, len );
    // TODO mutex!

    // Eats all positive sema value
    hal_sem_zero( &es->w_sem );

    es->w_buf = buf;
    es->w_len = len;

    hal_sem_acquire( &es->w_sem );

    return len;
#endif
}


#define RD_INT() *((u_int32_t*)buf)

static int es1370_ioctl(struct phantom_device *dev, int type, void *buf, int len)
{
    es1370_t *es = dev->drv_private;
    int tmp;

    if( len < (int)sizeof(u_int32_t) )
        return -1;

    switch(type)
    {
    case IOCTL_SOUND_SAMPLERATE:
        es->samplerate = RD_INT();
        set_sampling_rate(dev, es->samplerate);
        break;

    case IOCTL_SOUND_NCHANNELS:
        es->nchannels = RD_INT();
        goto setformat;
        //break;

    case IOCTL_SOUND_BITS:
        tmp = RD_INT();
        if( tmp != 8 && tmp != 16 )
            return -1; // TODO errno?
        es->nbits = tmp;
        goto setformat;
        //break;

    case IOCTL_SOUND_SIGNED:
        {
            tmp = RD_INT();
            tmp = tmp ? ~0 : 0; // All ones (masked below) for signed, zero otherwise

            u_int32_t mask = ~0; // All zeroes
            int shift = 32 - (es->nbits-1);

            es->silence = tmp & (mask >> shift);
            SHOW_FLOW(0, "%d bits silence = 0x%x", es->silence );
        }
        break;

    case IOCTL_SOUND_VOLUME:
        {
            es->volume = RD_INT();
            setCodec(CodecVolMasterL, es->volume);
            setCodec(CodecVolMasterR, es->volume);
        }
        break;


    default:
        return -1;
    }

    return 0;

setformat:
    setPlaybackFormat(dev, es->nchannels, es->nbits);
    setRecordFormat(  dev, es->nchannels, es->nbits);

    return 0;
}


#include <kernel/properties.h>





//static const char *pList = "sampleRate,bits,channels,signed,volume";
static const char *pList = "sampleRate";

static errno_t es1370_listproperties( struct phantom_device *dev, int nProperty, char *pValue, int vlen )
{
    (void) dev;

    if( nProperty > 0 ) return ENOENT;

    strlcpy( pValue, pList, vlen );
    return 0;
}


static errno_t	es1370_getproperty( struct phantom_device *dev, const char *pName, char *pValue, int vlen )
{
    es1370_t *es = dev->drv_private;

    if(0 == stricmp(pName, "samplerate"))
    {
        snprintf( pValue, vlen, "%d", es->samplerate );
        return 0;
    }

    return ENOTTY;
}

static errno_t	es1370_setproperty( struct phantom_device *dev, const char *pName, const char *pValue )
{
    es1370_t *es = dev->drv_private;

    if(0 == stricmp(pName, "samplerate"))
    {
        if( 1 != sscanf( pValue, "%d", &es->samplerate ) )
            return EINVAL;
        set_sampling_rate(dev, es->samplerate);
        return 0;
    }

    return ENOTTY;
}





#endif // HAVE_PCI

