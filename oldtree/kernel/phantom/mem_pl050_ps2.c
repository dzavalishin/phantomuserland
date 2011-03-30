#ifdef ARCH_arm
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Arm PL031 RTC driver.
 *
**/


#define DEBUG_MSG_PREFIX "pl050.ps2"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/driver.h>
#include <kernel/arm/devid.h>
#include <hal.h>
#include <arm/memio.h>

#include <ringbuf.h>

/*
 *
 * Based on code from "PL050 Primecell Keyboard, Mouse driver
 * Copyright (C) 2010 Amit Mahajan"
 *
 */

#include "mem_pl050_ps2.h"
#include "mem_pl050_keymap.h"

#if 0

/* Enable Rx irq */
void kmi_rx_irq_enable(unsigned long base)
{
    *(volatile unsigned long *)(base + PL050_KMICR) = KMI_RXINTR;
}

int kmi_data_read(unsigned long base)
{
    /* Check and return if data present */
    if (*(volatile unsigned long *)(base + PL050_KMISTAT) & KMI_RXFULL)
        return *(volatile unsigned long *)(base + PL050_KMIDATA);
    else
        return 0;
}

char kmi_keyboard_read(int c, struct keyboard_state *state)
{
    int keycode, shkeycode;
    int keynum;
    int extflag;
    int modmask;

    /* Special codes */
    switch (c) {
    case 0xF0:
        /* release */
        state->modifiers |= MODIFIER_RELEASE;
        return 0;
    case 0xE0:
        /* extended */
        state->modifiers |= MODIFIER_EXTENDED;
        return 0;
    case 0xE1:
        /* extended for 2 characters - only used for Break in mode 2 */
        state->modifiers |= MODIFIER_EXTENDED;
        state->modifiers |= MODIFIER_EXTENDED2;
        return 0;
    }

    extflag = 1;
    modmask = 0xFFFFFFFF;

    /* Is this a scan code? */
    if (c > 0 && c <= 0x9F)
    {
        keynum = scancode_mode2_extended[c];

        /* ignore unrecognised codes */
        if (!keynum)
        {
            state->modifiers &= ~MODIFIER_RELEASE;
            return 0;
        }

        /* is this an extended code? */
        if (state->modifiers & MODIFIER_EXTENDED)
        {
            keycode = keymap_uk2[keynum].ext_nomods;
            extflag = 0;
            state->modifiers &= ~MODIFIER_EXTENDED;
            if (!keycode)
            {
                state->modifiers &= ~MODIFIER_RELEASE;
                return 0;
            }
        }
        else if (state->modifiers & MODIFIER_EXTENDED2)
        {
            keycode = keymap_uk2[keynum].ext_nomods;
            extflag = 0;
            state->modifiers &= ~MODIFIER_EXTENDED2;
            if (!keycode)
            {
                state->modifiers &= ~MODIFIER_RELEASE;
                return 0;
            }
        }
        else
        {
            keycode = keymap_uk2[keynum].nomods;
            if (!keycode)
            {
                state->modifiers &= ~MODIFIER_RELEASE;
                return 0;
            }
        }

        /* handle shift */
        if (state->modifiers & MODIFIER_CAPSLK)
        {
            if (keycode >= 'a' && keycode <= 'z')
            {
                if (!(state->modifiers & MODIFIER_SHIFT))
                {
                    shkeycode = !extflag ? keymap_uk2[keynum].ext_shift : keymap_uk2[keynum].shift;
                    if (shkeycode)
                        keycode = shkeycode;
                }
            }
            else
            {
                if (state->modifiers & MODIFIER_SHIFT)
                {
                    shkeycode = !extflag ? keymap_uk2[keynum].ext_shift : keymap_uk2[keynum].shift;
                    if (shkeycode)
                        keycode = shkeycode;
                }
            }
        }
        else
        {
            if (state->modifiers & MODIFIER_SHIFT)
            {
                shkeycode = extflag ? keymap_uk2[keynum].ext_shift : keymap_uk2[keynum].shift;
                if (shkeycode)
                    keycode = shkeycode;
            }
        }

        /* handle the numeric keypad */
        if (keycode & MODIFIER_NUMLK)
        {
            keycode &= ~MODIFIER_NUMLK;

            if (state->modifiers & MODIFIER_NUMLK)
            {
                if (!(state->modifiers & MODIFIER_SHIFT))
                {
                    switch (keycode)
                    {
                    case KEYCODE_HOME:
                        keycode = '7';
                        break;
                    case KEYCODE_UP:
                        keycode = '8';
                        break;
                    case KEYCODE_PAGEUP:
                        keycode = '9';
                        break;
                    case KEYCODE_LEFT:
                        keycode = '4';
                        break;
                    case KEYCODE_CENTER:
                        keycode = '5';
                        break;
                    case KEYCODE_RIGHT:
                        keycode = '6';
                        break;
                    case KEYCODE_END:
                        keycode = '1';
                        break;
                    case KEYCODE_DOWN:
                        keycode = '2';
                        break;
                    case KEYCODE_PAGEDN:
                        keycode = '3';
                        break;
                    case KEYCODE_INSERT:
                        keycode = '0';
                        break;
                    case KEYCODE_DELETE:
                        keycode = '.';
                        break;
                    }
                }
                else
                    modmask = ~MODIFIER_SHIFT;
            }
        }

        /* modifier keys */
        switch (keycode)
        {
        case KEYCODE_LSHIFT:
            if (state->modifiers & MODIFIER_RELEASE)
                state->modifiers &= ~(MODIFIER_LSHIFT | MODIFIER_RELEASE);
            else
                state->modifiers |= MODIFIER_LSHIFT;
            return 0;

        case KEYCODE_RSHIFT:
            if (state->modifiers & MODIFIER_RELEASE)
                state->modifiers &= ~(MODIFIER_RSHIFT | MODIFIER_RELEASE);
            else
                state->modifiers |= MODIFIER_RSHIFT;
            return 0;

        case KEYCODE_LCTRL:
            if (state->modifiers & MODIFIER_RELEASE)
                state->modifiers &= ~(MODIFIER_LCTRL | MODIFIER_RELEASE);
            else
                state->modifiers |= MODIFIER_LCTRL;
            return 0;

        case KEYCODE_RCTRL:
            if (state->modifiers & MODIFIER_RELEASE)
                state->modifiers &= ~(MODIFIER_RCTRL | MODIFIER_RELEASE);
            else
                state->modifiers |= MODIFIER_RCTRL;
            return 0;

        case KEYCODE_ALT:
            if (state->modifiers & MODIFIER_RELEASE)
                state->modifiers &= ~(MODIFIER_ALT | MODIFIER_RELEASE);
            else
                state->modifiers |= MODIFIER_ALT;
            return 0;

        case KEYCODE_ALTGR:
            if (state->modifiers & MODIFIER_RELEASE)
                state->modifiers &= ~(MODIFIER_ALTGR | MODIFIER_RELEASE);
            else
                state->modifiers |= MODIFIER_ALTGR;
            return 0;

        case KEYCODE_CAPSLK:
            if (state->modifiers & MODIFIER_RELEASE)
                state->modifiers &= ~MODIFIER_RELEASE;
            else
            {
                state->modifiers ^= MODIFIER_CAPSLK;
                //__keyb_update_locks (state);
            }
            return 0;

        case KEYCODE_SCRLK:
            if (state->modifiers & MODIFIER_RELEASE)
                state->modifiers &= ~MODIFIER_RELEASE;
            else
            {
                state->modifiers ^= MODIFIER_SCRLK;
                //__keyb_update_locks (state);
            }
            return 0;

        case KEYCODE_NUMLK:
            if (state->modifiers & MODIFIER_RELEASE)
                state->modifiers &= ~MODIFIER_RELEASE;
            else
            {
                state->modifiers ^= MODIFIER_NUMLK;
                //__keyb_update_locks (state);
            }
            return 0;
        }

        if (state->modifiers & MODIFIER_RELEASE)
        {
            /* clear release condition */
            state->modifiers &= ~MODIFIER_RELEASE;
        }
        else
        {
            /* write code into the buffer */
            return keycode;
        }
        return 0;
    }

    return 0;
}

/*
 * Simple logic to interpret keyboard keys and shift keys
 * TODO: Add support for all the modifier keys
 *
 * Keyevents work in 3 phase manner, if you press 'A':
 * 1. scan code for 'A' is generated
 * 2. Key release event i.e KYBD_DATA_KEYUP
 * 3. scan code for 'A' again is generated
 */
char kmi_keyboard_read(unsigned long base, struct keyboard_state *state)
{
    int keynum, keycode = 0;

    /* Read Keyboard RX buffer */
    unsigned char data = kmi_data_read(base);

    /* if a key up occurred (key released) occured */
    if (data == KYBD_DATA_KEYUP) {
        state->keyup = 1;
        return 0;
    }
    else if (state->keyup){
        state->keyup = 0;

        /* Check if shift was lifted */
        if ((data == KYBD_DATA_SHIFTL) || (data == KYBD_DATA_SHIFTR)) {
            state->shift = 0;
        }
        else {
            /*	Find key number */
            keynum = scancode_mode2_extended[data];
            if(state->shift)
                keycode = keymap_uk2[keynum].shift;
            else
                keycode = keymap_uk2[keynum].nomods;

        }

    }
    else if ((data == KYBD_DATA_SHIFTL) || (data == KYBD_DATA_SHIFTR)) {
        state->shift = 1;
    }

    return (unsigned char)keycode;
}



#endif






















static void pl050_init( addr_t port, int div )
{
    // Turn off all
    W32( port + PL050_KMICR, 0 );

    // Read & ignore any interrupt req
    (void) R32( port + PL050_KMIIR );

    // Read & ignore any status info
    (void) R32( port + PL050_KMISTAT );

    // Set clock divisor
    W32( port + PL050_KMICLKDIV, div );

    // Enable port & rx interrupts
    W32( port + PL050_KMICR, KMI_EN|KMI_RXINTR );

}


















typedef struct {
    ringbuf_t           *rx;
    hal_sem_t           rx_sem;

    //hal_sem_t           tx_sem;
} pl050;



static void pl050_rx_interrupt( void *arg )
{
    phantom_device_t * dev = arg;
    pl050 *ps2 = (pl050 *)dev->drv_private;

    int istat = R32( dev->iobase + PL050_KMIIR );
    int kmistat = R32(dev->iobase + PL050_KMISTAT);

    if( istat & PL050_KMIIR_RX_INTR )
    {
        SHOW_FLOW( 9, "%s intr stat %x", dev->name, istat );
        if(kmistat & KMI_RXFULL )
        {
            char c = R8(dev->iobase + PL050_KMIDATA);
            SHOW_FLOW( 9, "%s rx stat %x c=%x '%c'", dev->name, kmistat, c, c );
            ring_put( ps2->rx, c );
            hal_sem_release(&ps2->rx_sem);
        }
    }


}


static int pl050_read_byte(phantom_device_t * dev)
{
    pl050 *ps2 = (pl050 *)dev->drv_private;

    while( ring_empty( ps2->rx ) )
        hal_sem_acquire(&ps2->rx_sem);

    int c = ring_get( ps2->rx );

    SHOW_FLOW( 9, "%s read byte c=%x '%c'", dev->name, c, c );

    return c;
}

static int pl050_has_rx_byte(phantom_device_t * dev)
{
    pl050 *ps2 = (pl050 *)dev->drv_private;
    return !ring_empty( ps2->rx );
}


static void pl050_write_byte(phantom_device_t * dev, int byte)
{
    //pl050 *ps2 = (pl050 *)dev->drv_private;

    while( !( R32(dev->iobase + PL050_KMISTAT) & KMI_TXEMPTY ) )
        hal_sleep_msec(1);
        //hal_sem_acquire(&ps2->rx_sem);

    W8(dev->iobase + PL050_KMIDATA, byte);
}


static int expect(phantom_device_t * dev, int val, int msec )
{
    //pl050 *ps2 = (pl050 *)dev->drv_private;

    SHOW_FLOW( 2, "%s expect %x", dev->name, val );
    while(msec-- > 0)
    {
        while( (msec > 0) && !pl050_has_rx_byte(dev) )
        {
            hal_sleep_msec(1);
            msec--;
        }

        if(!pl050_has_rx_byte(dev))
            break;

        int got = pl050_read_byte(dev);
        if( val == got )
        {
            SHOW_INFO( 7, "%s got reply %x", dev->name, val );
            return 1;
        }
        else
            SHOW_ERROR( 0, "%s expected %x, got %x", dev->name, val, got );
    }

    SHOW_ERROR( 0, "%s expected %x, timed out", dev->name, val );
    return 0;
}




// Bus is 24MHz, we need 8, so divide by 3. Must go to board definitions!
#define PL050_BUS_FREQ_DIVIDER 3





static phantom_device_t * common_pl050_probe( int port, int irq, int stage )
{
    (void) stage;

    //if( arm_id( port, 0x050, 0xB105F00D, 1 ) )        //return 0;        SHOW_ERROR0( 0, "id failed" );

    pl050_init( port, PL050_BUS_FREQ_DIVIDER );

    pl050 *ps2 = calloc( 1, sizeof(pl050) );
    assert( ps2 != 0 );

    ps2->rx = ring_alloc( 32 );
    hal_sem_init(&ps2->rx_sem, "pl050.rx");

    phantom_device_t * dev = calloc(1, sizeof(phantom_device_t));
    assert( dev != 0 );

    dev->name = "pl050.?";
    dev->seq_number = 0;
    dev->drv_private = ps2;

    //dev->dops.read = pl050_read;
    //dev->dops.write = pl050_write;

    dev->iobase = port;
    dev->irq = irq;
    //dev->iomem = ;
    //dev->iomemsize = ;

    if( hal_irq_alloc( irq, &pl050_rx_interrupt, dev, HAL_IRQ_SHAREABLE ) )
    {
        SHOW_ERROR( 0, "IRQ %d is busy", irq );
        goto free2;
    }


    return dev;

free2:
    free( dev );
//free1:

    hal_sem_destroy(&ps2->rx_sem);
    ring_free( ps2->rx );
    free( ps2 );

    return 0;
}


static int keyb_seq_number = 0;
phantom_device_t * driver_pl050_keyb_probe( int port, int irq, int stage )
{
    phantom_device_t * dev = common_pl050_probe( port, irq, stage );
    if( !dev )
        return 0;

    dev->name = "pl050.keyb";
    dev->seq_number = keyb_seq_number++;

    //dev->dops.read = pl050_keyb_read;

    SHOW_FLOW( 2, "%s reset", dev->name );
    pl050_write_byte( dev, KYBD_DATA_RESET );
    expect( dev, KYBD_DATA_RTR, 100 );
        ;
    return dev;
}


static int mouse_seq_number = 0;
phantom_device_t * driver_pl050_mouse_probe( int port, int irq, int stage )
{
    phantom_device_t * dev = common_pl050_probe( port, irq, stage );
    if( !dev )
        return 0;

    dev->name = "pl050.mouse";
    dev->seq_number = mouse_seq_number++;

    //dev->dops.read = pl050_mouse_read;

    SHOW_FLOW( 2, "%s reset", dev->name );
    pl050_write_byte( dev, MOUSE_DATA_RESET );
    expect( dev, MOUSE_DATA_ACK, 100 );
    expect( dev, MOUSE_DATA_RTR, 100 );

    pl050_write_byte( dev, MOUSE_DATA_ENABLE );
    expect( dev, MOUSE_DATA_ACK, 100 );

    return dev;
}













#endif

