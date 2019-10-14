#if !CONF_NEW_PS2
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Keyboard driver. Based on NewOS driver, Copyright 2001-2002, Travis Geiselbrecht.
 *
 *
**/

// TODO http://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html#ss1.1

#ifdef ARCH_ia32


#define DEBUG_MSG_PREFIX "ps2keyb"
#include <debug_ext.h>
#define debug_level_flow 4
#define debug_level_error 10
#define debug_level_info 10


#include <hal.h>
#include <ia32/pio.h>

#include <phantom_libc.h>
#include <sys/libkern.h>

#include <kernel/init.h>
#include <kernel/properties.h>
#include <kernel/drivers.h>
#include <kernel/snap_sync.h> // request_snap

#include <misc.h>
#include <event.h>
#include <video/zbuf.h>

#include <dev/key_event.h>

#include <device.h>


#include "console.h"


static int      keyb_init = 0;







static int  		leds;

static hal_sem_t        keyboard_sem;

#define BUF_LEN 256
const unsigned int 	keyboard_buf_len = BUF_LEN;
static _key_event 	keyboard_buf[BUF_LEN];
static unsigned int 	head, tail;



const u_int16_t pc_keymap_reg[128] = {
    /* 0x00 */ 0, KEY_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', KEY_BACKSPACE, KEY_TAB,
    /* 0x10 */ 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', KEY_RETURN, KEY_LCTRL, 'a', 's',
    /* 0x20 */ 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', KEY_LSHIFT, '\\', 'z', 'x', 'c', 'v',
    /* 0x30 */ 'b', 'n', 'm', ',', '.', '/', KEY_RSHIFT, '*', KEY_LALT, ' ', KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    /* 0x40 */ KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_PAD_NUMLOCK, KEY_SCRLOCK, KEY_PAD_7, KEY_PAD_8, KEY_PAD_9, KEY_PAD_MINUS, KEY_PAD_4, KEY_PAD_5, KEY_PAD_6, KEY_PAD_PLUS, KEY_PAD_1,
    /* 0x50 */ KEY_PAD_2, KEY_PAD_3, KEY_PAD_0, KEY_PAD_PERIOD, 0, 0, 0, KEY_F11, KEY_F12, 0, 0, 0, 0, 0, 0, 0,
};

const u_int16_t pc_keymap_e0[128] = {
    /* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_PAD_ENTER, KEY_RCTRL, 0, 0,
    /* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x30 */ 0, 0, 0, 0, 0, KEY_PAD_DIVIDE, 0, KEY_PRTSCRN, KEY_RALT, 0, 0, 0, 0, 0, 0, 0,
    /* 0x40 */ 0, 0, 0, 0, 0, 0, 0, KEY_HOME, KEY_ARROW_UP, KEY_PGUP, 0, KEY_ARROW_LEFT, 0, KEY_ARROW_RIGHT, 0, KEY_END,
    /* 0x50 */ KEY_ARROW_DOWN, KEY_PGDN, KEY_INS, 0, 0, 0, 0, 0, 0, 0, 0, KEY_LWIN, KEY_RWIN, KEY_MENU, 0, 0
};


// 0 - alnum, 1 - func, 2 - shift

const u_int16_t pc_keymap_isfunc_reg[128] = {
    /* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0,
    /* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,
    /* 0x30 */ 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 1, 1, 1, 1, 1,
    /* 0x40 */ 1, 1, 1, 1, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x50 */ 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
};

static const int pc_keymap_isfunc_e0[128] = {
    /* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0,
    /* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x30 */ 0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0,
    /* 0x40 */ 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1,
    /* 0x50 */ 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 1, 0, 0
};



static void wait_for_output(void)
{
    while(inb(0x64) & 0x2)
        ;
}

static void set_leds(void)
{
    wait_for_output();
    outb(0x60, 0xed);
    wait_for_output();
    outb(0x60, leds);
}




static int _keyboard_read(_key_event *buf, u_int32_t len)
{
    unsigned int saved_tail;
    u_int32_t copied_events = 0;
    u_int32_t copy_len;
    //int rc;

    if( !keyb_init )
    {
        hal_sleep_msec(500);
        return 0;
    }

    /* clamp the input len value */
    len = umin(len, keyboard_buf_len - 1);

retry:
    hal_sem_acquire(&keyboard_sem);

    saved_tail = tail;
    if(head == saved_tail)
        goto retry;

    // copy out of the buffer
    if(head < saved_tail)
        copy_len = umin(len, saved_tail - head);
    else
        copy_len = umin(len, keyboard_buf_len - head);

    memcpy(buf, &keyboard_buf[head], copy_len * sizeof(_key_event));

    copied_events = copy_len;
    head = (head + copy_len) % keyboard_buf_len;
/* coverity.com whines about this and we need no more that 1 event for a call, so just turn off for now
    if(head == 0 && saved_tail > 0 && copied_events < len)
    {
        // we wrapped around and have more bytes to read
        // copy the first part of the buffer
        copy_len = umin(saved_tail, len - copied_events);
        memcpy(&buf[copied_events], &keyboard_buf[0], copy_len * sizeof(_key_event));
        copied_events += copy_len;
        head = copy_len;
    }
*/

    return copied_events;
}





static void insert_in_buf(_key_event *event)
{

    // can't call there in interrupt!
    //if(keyb_event_mode) send_event_to_q(event);

    unsigned int temp_tail = tail;

    // see if the next char will collide with the head
    temp_tail++;
    temp_tail %= keyboard_buf_len;
    if(temp_tail == head) {
        // buffer overflow, ditch this char
        return;
    }
    keyboard_buf[tail].keycode = event->keycode;
    keyboard_buf[tail].modifiers = event->modifiers;
    keyboard_buf[tail].keychar = event->keychar;
    tail = temp_tail;

    hal_sem_release( &keyboard_sem );
}





//static void handle_keycode(unsigned char key)
void handle_keycode(unsigned char key)
{
    //int retval = INT_NO_RESCHEDULE;
    _key_event event;

    static char seen_e0 = 0;
    static char seen_e1 = 0;
    static char kbreak = 0;
    //static char e1_byte1;

    int has_char = 0;

    event.modifiers = 0;
    event.keycode   = 0;

    // look for special keys first
    if(key == 0xf0) {
        kbreak = 1;
    }
    else if(key == 0xe0)
    {
        seen_e0 = 1;
    }
    else if(key == 0xe1)
    {
        seen_e1 = 2; // e1 set has 2 bytes per key
    }
    else if(seen_e0)
    {
        // this is the character after e0, do the keycode lookup
        event.keycode = pc_keymap_e0[key & 0x7f];
        has_char = pc_keymap_isfunc_e0[key & 0x7f];
        seen_e0 = 0;
        goto get_key;

    }
    else if(seen_e1)
    {
#if 0
        if( seen_e1 == 2 ) e1_byte1 = key;
        if( seen_e1 == 1 ) key |= (e1_byte1 << 8); // compose 16-bit keycode
#endif
        seen_e1--;
    }
    else
    {
        // it was a regular key, do the keycode lookup
        event.keycode = pc_keymap_reg[key & 0x7f];
        has_char = pc_keymap_isfunc_reg[key & 0x7f];
        goto get_key;
    }

    // It was prefix byte
    return;

get_key:
    if(kbreak || (key & 0x80))
    {
        event.modifiers |= KEY_MODIFIER_UP;
        key &= 0x7f; // mask out the top bit
    }
    else
    {
        event.modifiers |= KEY_MODIFIER_DOWN;
    }
    kbreak = 0;

    // Got no key code? Nothing to process
    if(event.keycode == 0)
        return;


    // do some special checks here
    switch(event.keycode)
    {

    case KEY_F9:
        request_snap();
        break;

        // special stuff
    case KEY_F10:
    case KEY_PRTSCRN:
        //panic("Keyboard Requested Halt\n");
        //printf("print scrn\n");
        scr_zbuf_paint();
        return;

    case KEY_F11:
        phantom_shutdown(0);
        break;

    case KEY_F12:
        hal_cpu_reset_real();
        break;

    case KEY_PAD_MINUS:
        panic("Keyboard panic request - KEY_PAD_MINUS key");
        break;

    case KEY_SCRLOCK:
        if(event.modifiers & KEY_MODIFIER_DOWN)
        {
            ;
            //dbg_set_serial_debug(dbg_get_serial_debug()?0:1);
        }
        break;
    }

    event.keychar = (has_char < 2) ? event.keycode : 0;

    switch( event.keycode )
    {
    case KEY_RETURN:        event.keychar = '\n'; break;
    case KEY_ESC:           event.keychar = 0x1B; break;
    case KEY_TAB:           event.keychar = '\t'; break;
    //case KEY_BACKSPACE:     event.keychar = '\n'; break;

    case KEY_PAD_DIVIDE:    event.keychar = '/'; break;
    case KEY_PAD_MULTIPLY:  event.keychar = '*'; break;
    case KEY_PAD_MINUS:	event.keychar = '-'; break;
    case KEY_PAD_PLUS:      event.keychar = '+'; break;
    case KEY_PAD_ENTER:     event.keychar = '\n'; break;
    case KEY_PAD_PERIOD:    event.keychar = '.'; break;
    case KEY_PAD_0:         event.keychar = '0'; break;
    case KEY_PAD_1:         event.keychar = '1'; break;
    case KEY_PAD_2:         event.keychar = '2'; break;
    case KEY_PAD_3:         event.keychar = '3'; break;
    case KEY_PAD_4:         event.keychar = '4'; break;
    case KEY_PAD_5:         event.keychar = '5'; break;
    case KEY_PAD_6:         event.keychar = '6'; break;
    case KEY_PAD_7:         event.keychar = '7'; break;
    case KEY_PAD_8:         event.keychar = '8'; break;
    case KEY_PAD_9:         event.keychar = '9'; break;
    }

    insert_in_buf(&event);
}

static void handle_keyboard_interrupt(void)
{
    unsigned char key, status;

    status = inb(0x64);
    if( !(status & 0x01) ) return;

    key = inb(0x60);

    //SHOW_FLOW( 11, "key = 0x%x", key);

    handle_keycode(key);
}





































int phantom_dev_keyboard_getc(void);

/*
static int maininited = 0;

static void maininit()
{
    if(maininited) return;

    if( hal_sem_init(&keyboard_sem, "KBD") )
        panic("could not create keyboard sem!\n");
    maininited = 1;
}
*/

static int phantom_dev_keyboard_init(int irq)
{
    //maininit();
    if( hal_sem_init(&keyboard_sem, "KBD") )
        panic("could not create keyboard sem!");

    leds = 0;
    set_leds();

    head = tail = 0;

    errno_t rc = hal_irq_alloc( irq, (void *)handle_keyboard_interrupt, 0, HAL_IRQ_SHAREABLE );
    if( rc ) panic("could not get IRC for keyboard drv: %d", rc);

    SHOW_INFO0( 0, "interrupt driven keyboard driver is ready" );

    phantom_set_console_getchar( phantom_dev_keyboard_getc );

    return 0;
}


//---------------------------------------------------------------------------
// Properties
//---------------------------------------------------------------------------


static void * prop_valp(struct properties *ps, void *context, size_t offset ) { (void) ps; (void) context, (void) offset; return 0; }

static property_t proplist[] =
{
    { pt_int32, "leds", 0, &leds, 0, (void *)set_leds, 0, 0 },
};

static properties_t props = { ".dev", proplist, PROP_COUNT(proplist), prop_valp };


//---------------------------------------------------------------------------
// Device
//---------------------------------------------------------------------------



static int seq_number = 0;

phantom_device_t * driver_isa_ps2k_probe( int port, int irq, int stage )
{
    (void) port;
    (void) stage;

    if( seq_number || phantom_dev_keyboard_init(irq) )
        return 0;

    //hal_irq_alloc( PS2_INTERRUPT, phantom_dev_ps2_int_handler, 0, HAL_IRQ_SHAREABLE );

    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = "ps2-keyboard";
    dev->seq_number = seq_number++;

    dev->props = &props;
    dev->dops.listproperties = gen_dev_listproperties;
    dev->dops.getproperty = gen_dev_getproperty;
    dev->dops.setproperty = gen_dev_setproperty;

    keyb_init = 1;

    return dev;

}




int phantom_dev_keyboard_getc(void)
{
    _key_event buf;


    do { _keyboard_read( &buf, 1); }
    while(buf.modifiers != KEY_MODIFIER_DOWN);

    return buf.keychar;
}

// Used by event system
void phantom_dev_keyboard_get_key( _key_event *out)
{
    _keyboard_read( out, 1);
    lprintf("get key %d ch '%c' mod 0x%x ", out->keycode, out->keychar, out->modifiers );
}


#if 1

#define K_OBUF_FUL 	0x01		/* output (from keybd) buffer full */

// TODO rename - board_....
int phantom_scan_console_getc(void)
{
#if 1
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
#else

    volatile unsigned saved_tail;

    do {

        saved_tail = tail;

        while(head == saved_tail)
        {
            if((inb(0x64) & K_OBUF_FUL) == 0)
                continue;
            handle_keyboard_interrupt();
        }

        _keyboard_read( &buf, 1);
    }
    while(buf.modifiers != KEY_MODIFIER_DOWN);

#endif

}
#endif

int board_boot_console_getc(void)
{
    return phantom_scan_console_getc();
}


#endif // ARCH_ia32


#endif // !CONF_NEW_PS2
