/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * PC real time clock interface. Timer (periodic interrupts).
 *
 *
**/


#define DEBUG_MSG_PREFIX "rtc-i"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10



#include <x86/isa.h>
#include <i386/pio.h>


#include <phantom_libc.h>
#include <hal.h>
#include <time.h>

//#include "rtc.h"
#include <kernel/ia32/rtc.h>
#include <kernel/ia32/rtc_regs.h>

// Use RTC as main system clock

//#define RTC_LOCK      int ie = hal_save_cli()
//#define RTC_UNLOCK    if(ie) hal_sti()

static void rtc_interrupt(void *a);
//static void dump_rtc(void);


void init_rtc_timer_interrupts(void)
{
    char prev;

    assert(! hal_irq_alloc(8, rtc_interrupt, 0, 0) );

    //RTC_LOCK;

    // freq = 32768 >> (rate-1)
    int rate = 9; // 128 Hz
    //int rate = 14; // 4 Hz - for test purposes

    rate &= 0x0F; //rate must be above 2 and not over 15. (this is a safe-guard to be sure it isn't over 15)

    isa_rtc_nmi_off();
//dump_rtc();

    // Set rate

    //outb(0x70, 0x0A); //set index to register A
    //prev=inb(0x71);

    //get initial value of register A
    prev = isa_rtc_read_reg( 0x0A );

    //outb(0x70, 0x0A); //reset index to A
    //outb(0x71, (prev & 0xF0) | rate);

    //write only our rate to A. Note, rate is the bottom 4 bits.
    isa_rtc_write_reg( 0x0A, (prev & 0xF0) | rate );


    // Turn on regular interrupts

    //outb(0x70, 0x0B); //set the index to register B
    //prev=inb(0x71);

    //read the current value of register B
    prev = isa_rtc_read_reg( 0x0B );

    //outb(0x70, 0x0B); //set the index again(a read will reset the index to register D)
    //outb(0x71, prev | 0x40);

    //write the previous value or'd with 0x40. This turns on bit 6 of register B
    isa_rtc_write_reg( 0x0B, prev | 0x40 );

    isa_rtc_nmi_on();


    //RTC_UNLOCK;

}


static void rtc_interrupt(void *a)
{
    (void) a;

    //RTC_LOCK;

    // Ack RTC
    //outb(0x70, 0x0C); //select register C
    //inb(0x71); //just throw away contents.

    isa_rtc_nmi_off();
    isa_rtc_read_reg( 0x0C );
    isa_rtc_nmi_on();

    //RTC_UNLOCK;

    t0_revive();

#if RTC_DEBUG
    putchar('$');
#endif
#if DRIVE_SCHED_FROM_RTC
    phantom_scheduler_time_interrupt();
#endif

    int usec_per_tick = 1000000/128;
    //hal_time_tick(usec_per_tick);
    //hal_time_tick(10000); // lie a bit?

}

/*
static void dump_rtc(void)
{
    int i, v;
    for( i = 0; i < 0x100; i++ )
    {
        v = isa_rtc_read_reg( i );
        printf("RTC(%3x) = %4x\n", i, v );
    }
}
*/

