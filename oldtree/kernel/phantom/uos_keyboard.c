/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005 Serge Vakulenko
 *
 * Driver for PS/2 keyboard.
 *
 * Specifications of PS/2 keyboard interface are available
 * at http://www.computer-engineering.org/ps2keyboard/
 * 
**/

#define DEBUG_MSG_PREFIX "ps2.k"
#include <debug_ext.h>
#define debug_level_flow 1
#define debug_level_error 10
#define debug_level_info 1


#include <spinlock.h>

#include <ia32/pio.h>
#include <phantom_libc.h>

#include <kernel/device.h>
#include <kernel/snap_sync.h>
#include <kernel/drivers.h>
#include <hal.h>
#include <errno.h>

#include <threads.h>
#include <event.h>

#include <compat/uos.h>
#include <compat/uos/keyboard.h>
#include "uos_keyboard.h"
#include "uos_i8042.h"


static void keyboard_ps2_wait_event (keyboard_ps2_t *u, keyboard_event_t *data);


//---------------------------------------------------------------------------
// Local compat defs
//---------------------------------------------------------------------------

#define mutex_lock hal_spin_lock_cli
#define mutex_unlock hal_spin_unlock_sti

//---------------------------------------------------------------------------
// END - Local compat defs
//---------------------------------------------------------------------------




static keyboard_ps2_t ps2k;
static hal_sem_t keybd_sem;
//static int keyb_init = 0;

#if 0
#define SCAN_CTRL	0x1D	/* Control */
#define SCAN_LSHIFT	0x2A	/* Left shift */
#define SCAN_RSHIFT	0x36	/* Right shift */
#define SCAN_ALT	0x38	/* Right shift */
#define SCAN_DEL	0x53	/* Del */
#define SCAN_CAPS	0x3A	/* Caps lock */
#define SCAN_NUM	0x45	/* Num lock */
#define SCAN_LGUI	0x5B	/* Left Windows */
#define SCAN_RGUI	0x5C	/* Right Windows */
#endif

#define STATE_BASE	0
#define STATE_E0	1	/* got E0 */
#define STATE_E1	2	/* got E1 */
//#define STATE_E11D	3	/* got E1-1D or E1-9D */
//#define STATE_E114	4	/* got E1-1D or E1-9D */

#include "keyboard_mode2_tab.h"

static int
set_rate_delay (int cps, int msec)
{
    unsigned char param;

    if      (cps < 3)  param = 0x1f;	/* 2 chars/sec */
    else if (cps < 4)  param = 0x1a;	/* 3 chars/sec */
    else if (cps < 5)  param = 0x17;	/* 4 chars/sec */
    else if (cps < 6)  param = 0x14;	/* 5 chars/sec */
    else if (cps < 7)  param = 0x12;	/* 6 chars/sec */
    else if (cps < 10) param = 0x0f;	/* 8 chars/sec */
    else if (cps < 11) param = 0x0c;	/* 10 chars/sec */
    else if (cps < 13) param = 0x0a;	/* 12 chars/sec */
    else if (cps < 16) param = 0x08;	/* 15 chars/sec */
    else if (cps < 20) param = 0x06;	/* 17 chars/sec */
    else if (cps < 23) param = 0x04;	/* 21 chars/sec */
    else if (cps < 26) param = 0x02;	/* 24 chars/sec */
    else if (cps < 29) param = 0x01;	/* 27 chars/sec */
    else               param = 0x00;	/* 30 chars/sec */

    if      (msec > 800) param |= 0x60;	/* 1.0 sec */
    else if (msec > 600) param |= 0x40;	/* 0.75 sec */
    else if (msec > 400) param |= 0x20;	/* 0.5 sec */

    return i8042_kbd_command (KBDK_TYPEMATIC, param);
}

static int
set_leds (int leds)
{
    unsigned char param;

    param = 0;
    if (leds & KEYLED_NUM)    param |= KBDK_SETLED_NUM_LOCK;
    if (leds & KEYLED_CAPS)   param |= KBDK_SETLED_CAPS_LOCK;
    if (leds & KEYLED_SCROLL) param |= KBDK_SETLED_SCROLL_LOCK;

    return i8042_kbd_command (KBDK_SETLED, param);
}

/*
 * Process a byte, received from keyboard.
 * Return 1 when new event record is generated.
 */
static int
make_event (keyboard_ps2_t *u, keyboard_event_t *m, unsigned char byte)
{
    LOG_FLOW( 5, "-k %X ", byte);

    switch (byte) {
    case 0xE0:
        /* First part of two-byte sequence. */
        u->state = STATE_E0;
        return 0;
    case 0xE1:
        /* First byte of Pause sequence. */
        u->state = STATE_E1;
        return 0;

    case 0xF0: // Release prefix
        u->state_F0 = 1;
        return 0;
    }

    if( u->state == STATE_E1 )
    {
        // Note that there was F0 passing inside the 
        // sequence and it triggered u->state_F0
        u->state_F0 = 0; // reset it - we don't need

        if( byte == 0x14 ) u->state_E114 = 1;

        // ignore other keys in ths state?
        u->state = STATE_BASE;
        return 0;
    }

    if(u->state_E114)
    {
        if( u->state_F0 && (byte == 0x77) )
        {
            u->state_E114 = 0;
            u->state_F0 = 0;

            m->key = KEY_PAUSE;
            m->release = 0;
            return 1;
        }
        return 0;
    }

    switch(u->state)
    {
        default:
        case STATE_BASE: 
            //if( byte > sizeof(scan_to_key))     m->key = 0;            else                                
            m->key =  scan_to_key    [byte]; 
            break;
        case STATE_E0:   
            //if( byte > sizeof(scan_to_key_e0))  m->key = 0;            else                                
            m->key =  scan_to_key_e0 [byte]; 
            break;
    }
    u->state = STATE_BASE;

    switch (m->key) {
    case KEY_LCTRL:
        if(u->state_F0)  u->modifiers &= ~KEYMOD_LCTRL;
        else             u->modifiers |= KEYMOD_LCTRL;
        break;

    case KEY_RCTRL:
        if(u->state_F0)  u->modifiers &= ~KEYMOD_RCTRL;
        else             u->modifiers |= KEYMOD_RCTRL;
        break;
        
    case KEY_LSHIFT:
        if(u->state_F0)  u->modifiers &= ~KEYMOD_LSHIFT;
        else             u->modifiers |= KEYMOD_LSHIFT;
        break;

    case KEY_RSHIFT:
        if(u->state_F0)  u->modifiers &= ~KEYMOD_RSHIFT;
        else             u->modifiers |= KEYMOD_RSHIFT;
        break;
        
    case KEY_LALT:
        if(u->state_F0)  u->modifiers &= ~KEYMOD_LALT;
        else             u->modifiers |= KEYMOD_LALT;
        break;

    case KEY_RALT:
        if(u->state_F0)  u->modifiers &= ~KEYMOD_RALT;
        else             u->modifiers |= KEYMOD_RALT;
        break;

    case KEY_LMETA:
        if(u->state_F0)  u->modifiers &= ~KEYMOD_LMETA;
        else             u->modifiers |= KEYMOD_LMETA;
        break;

    case KEY_RMETA:
        if(u->state_F0)  u->modifiers &= ~KEYMOD_RMETA;
        else             u->modifiers |= KEYMOD_RMETA;
        break;
        
        
    // TODO scroll lock - national keytable?

        case KEY_CAPSLOCK:
            if(u->state_F0) return 0; // ignore release

            if (u->capslock) return 0;
            u->capslock = 1;

            if (u->modifiers & KEYMOD_CAPS) {
                u->modifiers &= ~KEYMOD_CAPS;
                u->leds &= ~KEYLED_CAPS;
            } else {
                u->modifiers |= KEYMOD_CAPS;
                u->leds |= KEYLED_CAPS;
            }
            set_leds (u->leds);
            break;

        case KEY_NUMLOCK:
            if(u->state_F0) return 0; // ignore release

            if (u->numlock) return 0;
            u->numlock = 1;

            if (u->modifiers & KEYMOD_NUM) {
                u->modifiers &= ~KEYMOD_NUM;
                u->leds &= ~KEYLED_NUM;
            } else {
                u->modifiers |= KEYMOD_NUM;
                u->leds |= KEYLED_NUM;
            }
            set_leds (u->leds);
            break;

        default:
        m->release = u->state_F0;
        u->state_F0 = 0;

        if (! m->key)        return 0;

        return 1;
    }

    u->state_F0 = 0;
    return 0;
}

/*
 * Process a byte, received from keyboard.
 * Return 1 when new event record is generated.
 */
static int
receive_byte (keyboard_ps2_t *u, unsigned char byte)
{
    keyboard_event_t *newlast;

    if( ((byte == KBDR_TEST_OK) || (byte == KBDR_ACK)) && u->state == STATE_BASE) {
        /* New device connected. */
        set_leds (u->leds);
        set_rate_delay (u->rate, u->delay);
        u->state = STATE_BASE;
        u->in_first = u->in_last = u->in_buf;
        return 0;
    }

    /* Advance queue pointer. */
    newlast = u->in_last + 1;
    if (newlast >= u->in_buf + KBD_INBUFSZ)
        newlast = u->in_buf;

    /* Ignore input on buffer overflow. */
    if (newlast == u->in_first)
        return 0;

    /* Make event record. */
    if (! make_event (u, u->in_last, byte))
        return 0;
    u->in_last->modifiers = u->modifiers;

    u->in_last = newlast;
    return 1;
}

/*
 * Process keyboard interrupt.
 * Get raw data and put to buffer. Return 1 when not enough data
 * for a event record.
 * Return 0 when a new event record is generated and
 * a signal to keyboard task is needed.
 */
static void
keyboard_ps2_interrupt (void *a)
{
    (void) a;
    keyboard_ps2_t *u = &ps2k;

    unsigned char c, sts, strobe;
    //int event_generated = 0;

    LOG_INFO0( 10, "" );

    /* Read the pending information. */
    for (;;) {
        sts = inb (KBDC_AT_CTL);
        if (! (sts & KBSTS_DATAVL))
            return; // (event_generated == 0);

        c = inb (KBD_DATA);
        if (sts & KBSTS_AUX_DATAVL) {
            /* This byte is for mouse.
             * Just ignore it for now (TODO). */
            continue;
        }

        /* Strobe the keyboard to ack the char. */
        strobe = inb (KBDC_XT_CTL);
        outb_reverse (strobe | KBDC_XT_CLEAR, KBDC_XT_CTL);
        outb_reverse (strobe, KBDC_XT_CTL);

        /*debug_printf ("<%02x> ", c);*/
        if (receive_byte (u, c))
        {
            hal_sem_release( &keybd_sem );
            //event_generated = 1;
        }
    }
}

/*
 * Keyb task.
 */
static void
keyboard_ps2_task (void)
{
    keyboard_ps2_t *u = &ps2k;

    //mutex_lock_irq (&u->lock, RECEIVE_IRQ,                    (handler_t) keyboard_ps2_interrupt, u);

    i8042_kbd_enable ();
    if (i8042_kbd_probe ()) {
        set_leds (u->leds);
        set_rate_delay (u->rate, u->delay);
    }

    for (;;) {
        //mutex_wait (&u->lock);        /* Nothing to do. */

        keyboard_event_t data;

        keyboard_ps2_wait_event( u, &data );

        // todo -  data.key = ascii_to_UTF_cyrillic[data.key], if(!shifts) tolower_cyrillic: keyboard_cyrillic_tab.h

        keyboard_translate( &data );


        int shifts = data.release ? UI_MODIFIER_KEYUP : 0;

        if( data.modifiers & KEYMOD_CTRL )      shifts |= UI_MODIFIER_CTRL;
        if( data.modifiers & KEYMOD_SHIFT )     shifts |= UI_MODIFIER_SHIFT;
        if( data.modifiers & KEYMOD_ALT )       shifts |= UI_MODIFIER_ALT;
        if( data.modifiers & KEYMOD_META )      shifts |= UI_MODIFIER_WIN;

        // TODO left/right keys - JUST REDEFINE OS defines accordoing to this driver's and assign

        vm_lock_persistent_memory();
        LOG_FLOW( 4, "vk=0x%x, ch=%c, shifts 0x%x ", data.key, data.key, shifts );
        //ev_q_put_key( event->keycode, event->keychar, shifts );
        ev_q_put_key( 0, data.key, shifts );
        vm_unlock_persistent_memory();
    }
}

/*
 * Read mouse movement data. Return immediately, do not wait.
 * Return 1 if data available, 0 if no data.
 * /
static int
keyboard_ps2_get_event (keyboard_ps2_t *u, keyboard_event_t *data)
{
    mutex_lock (&u->lock);
    if (u->in_first == u->in_last) {
        mutex_unlock (&u->lock);
        return 0;
    } else {
        *data = *u->in_first++;
        if (u->in_first >= u->in_buf + KBD_INBUFSZ)
            u->in_first = u->in_buf;
        mutex_unlock (&u->lock);
        return 1;
    }
} */

/*
 * Wait for the mouse movement and return it.
 */
static void
keyboard_ps2_wait_event (keyboard_ps2_t *u, keyboard_event_t *data)
{
    //mutex_lock (&u->lock);
again:
    /* Wait until receive data available. */
    while (u->in_first == u->in_last)
        hal_sem_acquire( &keybd_sem );

    LOG_INFO_( 9, "got evnt, u->in_first = %d", u->in_first - u->in_buf );

    mutex_lock (&u->lock);

    if(u->in_first == u->in_last)
    {
        mutex_unlock (&u->lock);
        goto again;
    }

    *data = *u->in_first++;
    if (u->in_first >= u->in_buf + KBD_INBUFSZ)
        u->in_first = u->in_buf;

    mutex_unlock (&u->lock);
}

#if 0
static int
keyboard_ps2_get_modifiers (keyboard_ps2_t *u)
{
    int modifiers;

    //mutex_lock (&u->lock);
    modifiers = u->modifiers;
    //mutex_unlock (&u->lock);
    return modifiers;
}

static int
keyboard_ps2_get_leds (keyboard_ps2_t *u)
{
    int leds;

    //mutex_lock (&u->lock);
    leds = u->leds;
    //mutex_unlock (&u->lock);
    return leds;
}

static void
keyboard_ps2_set_leds (keyboard_ps2_t *u, int leds)
{
    mutex_lock (&u->lock);

    u->leds = leds;
    set_leds (u->leds);

    mutex_unlock (&u->lock);
}

static int
keyboard_ps2_get_rate (keyboard_ps2_t *u)
{
    int rate;

    //mutex_lock (&u->lock);
    rate = u->rate;
    //mutex_unlock (&u->lock);
    return rate;
}

static void
keyboard_ps2_set_rate (keyboard_ps2_t *u, int cps)
{
    mutex_lock (&u->lock);

    if      (cps < 3)  u->rate = 2;
    else if (cps < 4)  u->rate = 3;
    else if (cps < 5)  u->rate = 4;
    else if (cps < 6)  u->rate = 5;
    else if (cps < 7)  u->rate = 6;
    else if (cps < 10) u->rate = 8;
    else if (cps < 11) u->rate = 10;
    else if (cps < 13) u->rate = 12;
    else if (cps < 16) u->rate = 15;
    else if (cps < 20) u->rate = 17;
    else if (cps < 23) u->rate = 21;
    else if (cps < 26) u->rate = 24;
    else if (cps < 29) u->rate = 27;
    else               u->rate = 30;

    set_rate_delay (u->rate, u->delay);

    mutex_unlock (&u->lock);
}

static int
keyboard_ps2_get_delay (keyboard_ps2_t *u)
{
    int delay;

    //mutex_lock (&u->lock);
    delay = u->delay;
    //mutex_unlock (&u->lock);
    return delay;
}

static void
keyboard_ps2_set_delay (keyboard_ps2_t *u, int msec)
{
    mutex_lock (&u->lock);

    if      (msec > 800) u->delay = 1000;
    else if (msec > 600) u->delay = 750;
    else if (msec > 400) u->delay = 500;
    else                 u->delay = 250;

    set_rate_delay (u->rate, u->delay);

    mutex_unlock (&u->lock);
}

/*
 static keyboard_interface_t keyboard_ps2_interface = {
 (void (*) (keyboard_t*, keyboard_event_t*))	keyboard_ps2_wait_event,
 (int (*) (keyboard_t*, keyboard_event_t*))	keyboard_ps2_get_event,
 (int (*) (keyboard_t*))				keyboard_ps2_get_modifiers,
 (int (*) (keyboard_t*))				keyboard_ps2_get_leds,
 (void (*) (keyboard_t*, int))			keyboard_ps2_set_leds,
 (int (*) (keyboard_t*))				keyboard_ps2_get_rate,
 (void (*) (keyboard_t*, int))			keyboard_ps2_set_rate,
 (int (*) (keyboard_t*))				keyboard_ps2_get_delay,
 (void (*) (keyboard_t*, int))			keyboard_ps2_set_delay,
 };
 */
#endif

static void
uos_keyboard_ps2_init (keyboard_ps2_t *u)
{
    //u->interface = &keyboard_ps2_interface;
    u->rate = 20;
    u->delay = 500;
    u->state = STATE_BASE;
    u->in_first = u->in_last = u->in_buf;

    /* Create keyboard receive task. */
    //task_create (keyboard_ps2_task, u, "kbd", prio,	u->stack, sizeof (u->stack));
}



//---------------------------------------------------------------------------
// Properties
//---------------------------------------------------------------------------

/*
static void * prop_valp(struct properties *ps, void *context, size_t offset ) { (void) ps; (void) context, (void) offset; return 0; }

static property_t proplist[] =
{
    { pt_int32, "leds", 0, &leds, 0, (void *)set_leds, 0, 0 },
};

static properties_t props = { ".dev", proplist, PROP_COUNT(proplist), prop_valp };
*/

//---------------------------------------------------------------------------
// Device
//---------------------------------------------------------------------------


static int seq_number = 0;

phantom_device_t * driver_isa_ps2k_probe( int port, int irq, int stage )
{
    (void) port;
    (void) stage;

    if( seq_number )        return 0;

    uos_keyboard_ps2_init(&ps2k);

    hal_sem_init( &keybd_sem, "MouseDrv" );

    hal_irq_alloc( irq, keyboard_ps2_interrupt, &ps2k, HAL_IRQ_SHAREABLE );
    LOG_INFO_( 1, "got IRQ %d", irq );

    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = "ps2-keyboard";
    dev->seq_number = seq_number++;

    //dev->props = &props;
    //dev->dops.listproperties = gen_dev_listproperties;
    //dev->dops.getproperty = gen_dev_getproperty;
    //dev->dops.setproperty = gen_dev_setproperty;

    hal_start_kernel_thread((void*)keyboard_ps2_task);

    //keyb_init = 1;

    return dev;
}


//---------------------------------------------------------------------------
// Direct access for wait for keypress
//---------------------------------------------------------------------------

#define K_OBUF_FUL 	0x01		/* output (from keybd) buffer full */

int board_boot_console_getc(void)
{
    // TODO PIECE OF JUNK!
    // just waits for some key - REDO!

	// [dz] erra reports that this code does not wait. Added attempt to read from both
	// registers to clear possible existing byte
	inb(0x64); inb(0x60);


    do {

        while((inb(0x64) & K_OBUF_FUL) == 0)
            ;
    } while( inb(0x60) & 0x80 );

    return 0;
}

