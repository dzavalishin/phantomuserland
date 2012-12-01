#ifdef ARCH_arm

/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2013 Dmitry Zavalishin, dz@dz.ru
 *
 * ARM raspberry PI (bcm2835) timer.
 *
**/



#define DEBUG_MSG_PREFIX "bcm2835timer"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <arm/memio.h>

#include <kernel/debug.h>
#include <hal.h>
#include <stdio.h>


#include "driver_arm_raspberry_interrupts.h"


#define BCM2835_TIMER_FINGERPRINT 0x544D5241

//#define BCM2835_TIMER_BASE      0x7E00B000
#define BCM2835_INTR_REG_BASE    (0x20000000+0xB000)

#define BCM2835_TIMER_LOAD      (BCM2835_TIMER_BASE+0x400)
#define BCM2835_TIMER_VALUE     (BCM2835_TIMER_BASE+0x404)
#define BCM2835_TIMER_CTRL      (BCM2835_TIMER_BASE+0x408)
#define BCM2835_TIMER_ACK       (BCM2835_TIMER_BASE+0x40C)

#define BCM2835_TIMER_RAW_IRQ   (BCM2835_TIMER_BASE+0x410)
#define BCM2835_TIMER_MSK_IRQ   (BCM2835_TIMER_BASE+0x414)
#define BCM2835_TIMER_RELOAD    (BCM2835_TIMER_BASE+0x418)


#define BCM2835_TIMER_CTL_ENA   0x80
#define BCM2835_TIMER_CTL_INTR  0x20

#define BCM2835_TIMER_CTL_DIV1   (0<<2)
#define BCM2835_TIMER_CTL_DIV16  (1<<2)
#define BCM2835_TIMER_CTL_DIV256 (2<<2)

#define BCM2835_TIMER_CTL_32BIT 0x2

#define SETUP (BCM2835_TIMER_CTL_32BIT|BCM2835_TIMER_CTL_ENA|BCM2835_TIMER_CTL_DIV1)

static int usec_per_tick = 10000; // 100 Hz



static void arm_raspberry_timer0_int_handler(void *arg)
{
    (void) arg;

    W32( BCM2835_TIMER_ACK, 0 );
    //putchar('|');
    hal_time_tick(usec_per_tick);
}



void arm_raspberry_timer0_init(int freq)
{
    if( hal_irq_alloc( ARM_INT_TIMER, &arm_raspberry_timer0_int_handler, 0, HAL_IRQ_SHAREABLE ) )
    {
        SHOW_ERROR( 0, "IRQ %d is busy", ARM_INT_TIMER );
        return;
    }

    usec_per_tick = 1000000/freq;

    int raspberry_tmr_count = 1000000/freq; // 1 MHz/freq

    W32( BCM2835_TIMER_LOAD, raspberry_tmr_count );
    W32( BCM2835_TIMER_CTRL, SETUP|BCM2835_TIMER_CTL_INTR );
}






#endif // ARCH_arm

