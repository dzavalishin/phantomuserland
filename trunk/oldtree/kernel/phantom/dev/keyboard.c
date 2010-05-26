/*
 ** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */


#include <hal.h>
#include <i386/pio.h>

#include <phantom_libc.h>

#include <misc.h>
#include <event.h>

#include "dev/key_event.h"

#include "driver_map.h"
#include "device.h"

#include "console.h"


/* TODO: MOVE THIS TO A PRIVATE HEADER FILE */
#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif


static int  leds;
static hal_cond_t keyboard_sem;
static hal_mutex_t  keyboard_read_mutex;
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

    /* clamp the input len value */
    len = min(len, keyboard_buf_len - 1);

retry:
    // critical section
    hal_mutex_lock(&keyboard_read_mutex);

    // block here until data is ready
    hal_cond_wait( &keyboard_sem, &keyboard_read_mutex );


    saved_tail = tail;
    if(head == saved_tail) {
        hal_mutex_unlock(&keyboard_read_mutex);
        goto retry;
    } else {
        // copy out of the buffer
        if(head < saved_tail)
            copy_len = min(len, saved_tail - head);
        else
            copy_len = min(len, keyboard_buf_len - head);
        memcpy(buf, &keyboard_buf[head], copy_len * sizeof(_key_event));
        copied_events = copy_len;
        head = (head + copy_len) % keyboard_buf_len;
        if(head == 0 && saved_tail > 0 && copied_events < len) {
            // we wrapped around and have more bytes to read
            // copy the first part of the buffer
            copy_len = min(saved_tail, len - copied_events);
            memcpy(&buf[copied_events], &keyboard_buf[0], copy_len * sizeof(_key_event));
            copied_events += copy_len;
            head = copy_len;
        }
    }
    if(head != saved_tail) {
        // we did not empty the keyboard queue
        hal_cond_broadcast( &keyboard_sem );
    }

    hal_mutex_unlock(&keyboard_read_mutex);

    return copied_events;
}


static int transfer_to_event = 0;

void phantom_dev_keyboard_start_events()
{
    transfer_to_event = 1;
}

static void insert_in_buf(_key_event *event)
{
    if(transfer_to_event)
    {
        static int shifts;

        int dn = event->modifiers & KEY_MODIFIER_DOWN;

        if( event->modifiers & KEY_MODIFIER_UP )
            shifts |= UI_MODIFIER_KEYUP;

        if( dn )
            shifts &= ~UI_MODIFIER_KEYUP;

        switch(event->keycode)
        {
        case KEY_LSHIFT:
            shifts &= ~UI_MODIFIER_LSHIFT;
            if(dn)
                shifts |= UI_MODIFIER_LSHIFT;

        set_comon_shift:
            shifts &= ~UI_MODIFIER_SHIFT;
            if( (shifts & UI_MODIFIER_LSHIFT) || (shifts & UI_MODIFIER_RSHIFT) )
                shifts |= UI_MODIFIER_SHIFT;
            break;

        case KEY_RSHIFT:
            shifts &= ~UI_MODIFIER_RSHIFT;
            if(dn)
                shifts |= UI_MODIFIER_RSHIFT;

            goto set_comon_shift;




        case KEY_LCTRL:
            shifts &= ~UI_MODIFIER_LCTRL;
            if(dn)
                shifts |= UI_MODIFIER_LCTRL;

        set_comon_ctrl:
            shifts &= ~UI_MODIFIER_CTRL;
            if( (shifts & UI_MODIFIER_LCTRL) || (shifts & UI_MODIFIER_RCTRL ) )
                shifts |= UI_MODIFIER_CTRL;
            break;

        case KEY_RCTRL:
            shifts &= ~UI_MODIFIER_RCTRL;
            if(dn)
                shifts |= UI_MODIFIER_RCTRL;

            goto set_comon_ctrl;



        case KEY_LALT:
            shifts &= ~UI_MODIFIER_LALT;
            if(dn)
                shifts |= UI_MODIFIER_LALT;

        set_comon_alt:
            shifts &= ~UI_MODIFIER_ALT;
            if( (shifts & UI_MODIFIER_LALT) || (shifts & UI_MODIFIER_RALT ) )
                shifts |= UI_MODIFIER_ALT;
            break;

        case KEY_RALT:
            shifts &= ~UI_MODIFIER_RALT;
            if(dn)
                shifts |= UI_MODIFIER_RALT;

            goto set_comon_alt;


        case KEY_LWIN:
            if(dn)              shifts |= UI_MODIFIER_LWIN;
            else             	shifts &= ~UI_MODIFIER_LWIN;

        set_comon_win:
            if( (shifts & UI_MODIFIER_LWIN) || (shifts & UI_MODIFIER_RWIN ) )
                shifts |= UI_MODIFIER_WIN;
            else
                shifts &= ~UI_MODIFIER_WIN;
            break;

        case KEY_RWIN:
            if(dn)              shifts |= UI_MODIFIER_RWIN;
            else                shifts &= ~UI_MODIFIER_RWIN;
            goto set_comon_win;




        case KEY_CAPSLOCK:
            if(dn)
            {
                if(shifts & UI_MODIFIER_CAPSLOCK)
                    shifts &= ~UI_MODIFIER_CAPSLOCK;
                else
                    shifts |= UI_MODIFIER_CAPSLOCK;
            }

        case KEY_SCRLOCK:
            if(dn)
            {
                if(shifts & UI_MODIFIER_SCRLOCK)
                    shifts &= ~UI_MODIFIER_SCRLOCK;
                else
                    shifts |= UI_MODIFIER_SCRLOCK;
            }

        case KEY_PAD_NUMLOCK:
            if(dn)
            {
                if(shifts & UI_MODIFIER_NUMLOCK)
                    shifts &= ~UI_MODIFIER_NUMLOCK;
                else
                    shifts |= UI_MODIFIER_NUMLOCK;
            }

        }

        event_q_put_key( event->keycode, event->keychar, shifts );
        return;
    }

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
    hal_cond_broadcast( &keyboard_sem );
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
        case KEY_PRTSCRN:
            panic("Keyboard Requested Halt\n");
            break;
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






int phantom_dev_keyboard_getc(void);

int direct_trygetchar_hook_active;

static int maininited = 0;

static void maininit()
{
    if(maininited) return;

    if( hal_cond_init(&keyboard_sem, "KBD") )
        panic("could not create keyboard sem!\n");

    if( hal_mutex_init(&keyboard_read_mutex, "KBD") )
        panic("could not create keyboard read mutex!\n");

    maininited = 1;
}


int phantom_dev_keyboard_init(int irq)
{
    maininit();
    leds = 0;
    set_leds();

    head = tail = 0;

    // todo - NOT shareable, really?
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
    dev->name = "PS/2 keyboard";
    dev->seq_number = seq_number++;

    return dev;

}



#if 0
int direct_trygetchar_hook_trygetchar(void)
{
    unsigned int saved_tail;
    _key_event buf;

    //printf("interrupt driven keyboard hook is called\n");
again:
    // critical section
    hal_mutex_lock(&keyboard_read_mutex);

    saved_tail = tail;
    if(head == saved_tail) {
        hal_mutex_unlock(&keyboard_read_mutex);
        return -1;
    } else {
        memcpy(&buf, &keyboard_buf[head], sizeof(_key_event) );

        head = (head + 1) % keyboard_buf_len;
    }
    if(head != saved_tail) {
        // we did not empty the keyboard queue
        hal_cond_broadcast( &keyboard_sem );
    }

    hal_mutex_unlock(&keyboard_read_mutex);

    if(buf.modifiers != KEY_MODIFIER_DOWN)
        goto again;

    return buf.keychar;
}
#endif

int phantom_dev_keyboard_getc(void)
{
    _key_event buf;


    do { _keyboard_read( &buf, 1); }
    while(buf.modifiers != KEY_MODIFIER_DOWN);

    return buf.keychar;
}


#define K_OBUF_FUL 	0x01		/* output (from keybd) buffer full */

int phantom_scan_console_getc(void)
{
#if 1
    // TODO PIECE OF JUNK!
    // just wait some key - REDO!

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

