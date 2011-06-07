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

#ifdef ARCH_ia32

//---------------------------------------------------------------------------

#define DEBUG_MSG_PREFIX "ps2keyb"
#include <debug_ext.h>
#define debug_level_flow 7
#define debug_level_error 10
#define debug_level_info 10


#include <hal.h>
#include <i386/pio.h>

#include <phantom_libc.h>
#include <sys/libkern.h>
#include <kernel/init.h>

#include <misc.h>
#include <event.h>

#include <dev/key_event.h>

//#include "driver_map.h"
#include <device.h>
#include <kernel/drivers.h>

#include "console.h"


static int      keyb_init = 0;



#define KEYB_EVENT_PUSH_THREAD 0
#define KEYB_USE_SEMA 1

#define USE_SOFTIRQ 0

#if USE_SOFTIRQ
static int softirq = -1;
#endif




static int  leds;

#if KEYB_USE_SEMA
static hal_sem_t        keyboard_sem;
#else
static hal_cond_t keyboard_sem;
static hal_mutex_t  keyboard_read_mutex;
#endif

#define BUF_LEN 256
const unsigned int keyboard_buf_len = BUF_LEN;
static _key_event keyboard_buf[BUF_LEN];
static unsigned int head, tail;
//static isa_bus_manager *isa;



const u_int16_t pc_keymap_set1[128] = {
    /* 0x00 */ 0, KEY_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', KEY_BACKSPACE, KEY_TAB,
    /* 0x10 */ 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', KEY_RETURN, KEY_LCTRL, 'a', 's',
    /* 0x20 */ 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', KEY_LSHIFT, '\\', 'z', 'x', 'c', 'v',
    /* 0x30 */ 'b', 'n', 'm', ',', '.', '/', KEY_RSHIFT, '*', KEY_LALT, ' ', KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    /* 0x40 */ KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_PAD_NUMLOCK, KEY_SCRLOCK, KEY_PAD_7, KEY_PAD_8, KEY_PAD_9, KEY_PAD_MINUS, KEY_PAD_4, KEY_PAD_5, KEY_PAD_6, KEY_PAD_PLUS, KEY_PAD_1,
    /* 0x50 */ KEY_PAD_2, KEY_PAD_3, KEY_PAD_0, KEY_PAD_PERIOD, 0, 0, 0, KEY_F11, KEY_F12, 0, 0, 0, 0, 0, 0, 0,
};

const u_int16_t pc_keymap_set1_e0[128] = {
    /* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_PAD_ENTER, KEY_RCTRL, 0, 0,
    /* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x30 */ 0, 0, 0, 0, 0, KEY_PAD_DIVIDE, 0, KEY_PRTSCRN, KEY_RALT, 0, 0, 0, 0, 0, 0, 0,
    /* 0x40 */ 0, 0, 0, 0, 0, 0, 0, KEY_HOME, KEY_ARROW_UP, KEY_PGUP, 0, KEY_ARROW_LEFT, 0, KEY_ARROW_RIGHT, 0, KEY_END,
    /* 0x50 */ KEY_ARROW_DOWN, KEY_PGDN, KEY_INS, 0, 0, 0, 0, 0, 0, 0, 0, KEY_LWIN, KEY_RWIN, KEY_MENU, 0, 0
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
        hal_sleep_msec(2000);
        return 0;
    }

    /* clamp the input len value */
    len = umin(len, keyboard_buf_len - 1);

retry:
#if KEYB_USE_SEMA
    hal_sem_acquire(&keyboard_sem);
#else
    // critical section
    hal_mutex_lock(&keyboard_read_mutex);

    // block here until data is ready
    hal_cond_wait( &keyboard_sem, &keyboard_read_mutex );
#endif

    saved_tail = tail;
    if(head == saved_tail) {
#if !KEYB_USE_SEMA
        hal_mutex_unlock(&keyboard_read_mutex);
#endif
        goto retry;
    } else {
        // copy out of the buffer
        if(head < saved_tail)
            copy_len = umin(len, saved_tail - head);
        else
            copy_len = umin(len, keyboard_buf_len - head);
        memcpy(buf, &keyboard_buf[head], copy_len * sizeof(_key_event));
        copied_events = copy_len;
        head = (head + copy_len) % keyboard_buf_len;
        if(head == 0 && saved_tail > 0 && copied_events < len) {
            // we wrapped around and have more bytes to read
            // copy the first part of the buffer
            copy_len = umin(saved_tail, len - copied_events);
            memcpy(&buf[copied_events], &keyboard_buf[0], copy_len * sizeof(_key_event));
            copied_events += copy_len;
            head = copy_len;
        }
    }
    if(head != saved_tail) {
#if !KEYB_USE_SEMA
        // we did not empty the keyboard queue
        hal_cond_broadcast( &keyboard_sem );
#endif
    }

#if !KEYB_USE_SEMA
    hal_mutex_unlock(&keyboard_read_mutex);
#endif

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

#if USE_SOFTIRQ
    hal_request_softirq( softirq );
#else

#if KEYB_USE_SEMA
    hal_sem_release( &keyboard_sem );
#else
    hal_cond_broadcast( &keyboard_sem );
#endif

#endif

}

static void handle_set1_keycode(unsigned char key)
{
    //int retval = INT_NO_RESCHEDULE;
    _key_event event;
    char queue_event = 1; // we will by default queue the key
    static char seen_e0 = 0;
    static char seen_e1 = 0;

    event.modifiers = 0;

    // look for special keys first
    if(key == 0xe0) {
        seen_e0 = 1;
        //		dprintf("handle_set1_keycode: e0 prefix\n");
        queue_event = 0;
    } else if(key == 0xe1) {
        seen_e1 = 1;
        //		dprintf("handle_set1_keycode: e1 prefix\n");
        queue_event = 0;
    } else if(seen_e0) {
        // this is the character after e0
        if(key & 0x80) {
            event.modifiers |= KEY_MODIFIER_UP;
            key &= 0x7f; // mask out the top bit
        } else {
            event.modifiers |= KEY_MODIFIER_DOWN;
        }

        // do the keycode lookup
        event.keycode = pc_keymap_set1_e0[key];

        if(event.keycode == 0) {
            queue_event = 0;
        }
        seen_e0 = 0;
    } else if(seen_e1) {
        seen_e1 = 0;
    } else {
        // it was a regular key
        if(key & 0x80) {
            event.modifiers |= KEY_MODIFIER_UP;
            key &= 0x7f; // mask out the top bit
        } else {
            event.modifiers |= KEY_MODIFIER_DOWN;
        }

        // do the keycode lookup
        event.keycode = pc_keymap_set1[key];

        // by default we're gonna queue this thing

        if(event.keycode == 0) {
            // some invalid key
            //			dprintf("handle_set1_keycode: got invalid raw key 0x%x\n", key);
            queue_event = 0;
        }
    }

    if(queue_event) {
        // do some special checks here
        switch(event.keycode) {
            // special stuff
        case KEY_F11:
        case KEY_PRTSCRN:
            //panic("Keyboard Requested Halt\n");
            printf("print scrn\n");
            video_zbuf_paint();
            return;
        case KEY_F12:
            hal_cpu_reset_real();
            break;
        case KEY_SCRLOCK:
            if(event.modifiers & KEY_MODIFIER_DOWN)
            {
                ;
                //dbg_set_serial_debug(dbg_get_serial_debug()?0:1);
            }
            break;
        }

        event.keychar = event.keycode;

        switch( event.keycode )
        {
        case KEY_RETURN:        event.keychar = '\n'; break;
        case KEY_ESC:        	event.keychar = 0x1B; break;
        case KEY_TAB:        	event.keychar = '\t'; break;
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
        //retval = INT_RESCHEDULE;
    }

    //return 0;
}

static void handle_keyboard_interrupt(void)
{
    unsigned char key;

    key = inb(0x60);

    //printf("handle_keyboard_interrupt: key = 0x%x\n", key);

    handle_set1_keycode(key);
}
























#if KEYB_EVENT_PUSH_THREAD
static int keyb_event_mode = 0;
static void keyb_event_loop( void *arg );

void phantom_dev_keyboard_start_events()
{
    if(!keyb_event_mode)
        hal_start_kernel_thread((void*)keyb_event_loop);

    keyb_event_mode = 1;
}
#endif











#if USE_SOFTIRQ
void softirq_handler( void *_arg )
{
    (void) _arg;
#if KEYB_USE_SEMA
    hal_sem_release( &keyboard_sem );
#else
    hal_cond_broadcast( &keyboard_sem );
#endif
}
#endif





int phantom_dev_keyboard_getc(void);

int direct_trygetchar_hook_active;

static int maininited = 0;

static void maininit()
{
    if(maininited) return;

#if USE_SOFTIRQ
    softirq = hal_alloc_softirq();
    if( softirq < 0 )
        panic( "Unable to get softirq" );

    hal_set_softirq_handler( softirq, &softirq_handler, 0 );
#endif

#if KEYB_USE_SEMA
    if( hal_sem_init(&keyboard_sem, "KBD") )
        panic("could not create keyboard sem!\n");
#else
    if( hal_cond_init(&keyboard_sem, "KBD") )
        panic("could not create keyboard cond!\n");

    if( hal_mutex_init(&keyboard_read_mutex, "KBD") )
        panic("could not create keyboard read mutex!\n");
#endif
    maininited = 1;
}


static int phantom_dev_keyboard_init(int irq)
{
    maininit();
    leds = 0;
    set_leds();

    head = tail = 0;

    hal_irq_alloc( irq, (void *)handle_keyboard_interrupt, 0, HAL_IRQ_SHAREABLE );

    direct_trygetchar_hook_active = 1;

    //printf("interrupt driven keyboard driver is ready\n");

    phantom_set_console_getchar( phantom_dev_keyboard_getc );

    return 0;
}



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
}


#if 1

#define K_OBUF_FUL 	0x01		/* output (from keybd) buffer full */

int phantom_scan_console_getc(void)
{
#if 1
    // TODO PIECE OF JUNK!
    // just waits for some key - REDO!

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


#if KEYB_EVENT_PUSH_THREAD
static void keyb_event_loop( void *arg )
{
    (void)arg;

    hal_set_thread_name("KeyEvents");
	hal_set_current_thread_priority(PHANTOM_SYS_THREAD_PRIO);

    while(1)
    {
        while(!keyb_event_mode)
            hal_sleep_msec(10000);

        _key_event buf;

        _keyboard_read( &buf, 1);

        //printf( "-- key ev --\n" );
        send_event_to_q( &buf );
    }

}
#endif


#endif // ARCH_ia32

