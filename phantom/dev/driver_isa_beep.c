/*!
 *      \brief System speaker driver.
 *      \author Mostly based on driver from Andrea Righi <drizzt@inwind.it>
 */

#define DEBUG_MSG_PREFIX "beep"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/drivers.h>

#include <i386/pio.h>
#include <phantom_libc.h>

#include <hal.h>
#include <kernel/timedcall.h>

//! Default beep frequency
#define DRV_BEEP_FREQ        0x440

//! Default beep duration (msec)
#define DRV_BEEP_TIME        20


//#define TIMER0          0x40    //!< I/O port for timer channel 0.
#define TIMER2          0x42    //!< I/O port for timer channel 2.
#define TIMER_MODE      0x43    //!< I/O port for timer mode control.
//#define SQUARE_WAVE     0x36    //!< The sqare-wave form.
#define TIMER_FREQ      1193182L //!< Clock frequency for timer in PC.



//! \brief Start a sound using the speaker.
//! \param frequency The frequency of the sound.
void sound(u_int32_t frequency)
{
	int ie;
	u_int32_t div;

    if( (frequency<19) || (frequency>22000) )
        return;

    div = TIMER_FREQ / frequency;

    ie = hal_save_cli();

    outb( 0x61, inb(0x61) | 3 );
    outb( TIMER_MODE, 0xB6);
    outb( TIMER2, div & 0xFF);
    outb( TIMER2, div >> 8);

    if(ie) hal_sti();
}

//! \brief Turn off the speaker.
void nosound()
{
    int ie = hal_save_cli();

    outb(0x61, inb(0x61) & 0xFC);

    if(ie) hal_sti();
}

//! \brief Play a system beep.
void beep()
{
    sound(DRV_BEEP_FREQ);
    phantom_request_timed_func( nosound, 0, DRV_BEEP_TIME, 0 );
}




// Stop device
static int beep_stop(phantom_device_t *dev)
{
    (void) dev;
    nosound();
    return 0;
}



static int beep_read(struct phantom_device *dev, void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;
    return -EIO;
}

static int beep_write(struct phantom_device *dev, const void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;
    // TODO process MIDI stream?
    return -EIO;
}



// TODO this is to be moved to driver_map.c and be kept in driver tables
static int seq_number = 0;

phantom_device_t * driver_isa_beep_probe( int port, int irq, int stage )
{
	phantom_device_t * dev;

    (void) port;
    (void) irq;
    (void) stage;

    if(seq_number)        return 0; // just one instance!

    // TODO check if we really have this hardware. How?

    dev = malloc(sizeof(phantom_device_t));
    dev->name = "BEEP";
    dev->seq_number = seq_number++;

    dev->dops.stop = beep_stop;

    dev->dops.read = beep_read;
    dev->dops.write = beep_write;

    return dev;
}




