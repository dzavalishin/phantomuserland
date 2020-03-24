/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * PC timer (8254)
 *
**/

#define PIT_OVERFLOW_HACK 0

#define DEBUG_MSG_PREFIX "pit8254"
#include "debug_ext.h"
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 0


#include <ia32/pio.h>
#include <phantom_libc.h>

#include <hal.h>
#include <time.h>

#include <dev/isa/pit_regs.h>
#include <kernel/bus/isa/pic.h>

#include "ia32drv.h"


int	hz = HZ;		/* number of ticks per second */


static int pitctl_port  = PITCTL;		/* For 386/20 Board */
static int pitctr0_port = PITCTR0;	/* For 386/20 Board */
//static int pitctr1_port = PITCTR1;	/* For 386/20 Board */
//static int pitctr2_port = PITCTR2;	/* For 386/20 Board */

/* We want PIT 0 in square wave mode */
static int pit0_mode = PIT_C0|PIT_NDIVMODE|PIT_READMODE ;


static unsigned int delaycount;		/* loop count in trying to delay for 1 millisecond */

// Used in asm code
unsigned long ten_microsec_sleep_loop_count = 50;		/* loop count for 10 microsecond wait.
MUST be initialized for those who
insist on calling "tenmicrosec"
it before the clock has been
initialized.
*/

#define uWAIT 0



/* number of milliseconds to delay */
void phantom_spinwait(int millis)
{
    volatile int i;
    volatile unsigned int j;

    for (i=0;i<millis;i++)
        for (j=0;j<delaycount;j++)
            ;
}



#define COUNT   10000   /* should be a multiple of 1000! */
#define PIT_COUNTDOWN PIT_READMODE|PIT_NDIVMODE

static void findspeed()
{
    unsigned char byte;
    unsigned int leftover;

    int s = hal_save_cli(); 		// disable interrupts

    /* Put counter in count down mode */
    outb(pitctl_port, PIT_COUNTDOWN);

    /* output a count of -1 to counter 0 */
    outb(pitctr0_port, 0xff);
    outb(pitctr0_port, 0xff);
    delaycount = COUNT;
    phantom_spinwait(1);

    /* Read the value left in the counter */
    byte = inb(pitctr0_port);		// least siginifcant
    leftover = inb(pitctr0_port);	// most significant
    leftover = (leftover<<8) + byte ;

    /*
     * Formula for delaycount is :
     *  (loopcount * timer clock speed)/ (counter ticks * 1000)
     * 1000 is for figuring out milliseconds
     */

    /* we arrange calculation so that it doesn't overflow */
    delaycount = ((COUNT/1000) * CLKNUM) / (0xffff-leftover);

    SHOW_FLOW( 1, "findspeed: delaycount=%d (tics=%d)", delaycount, (0xffff-leftover));
    
    if(s) hal_sti();                    // restore interrupt state
}




#define MICROCOUNT      1000    /* keep small to prevent overflow */
static void microfind()
{
    //unsigned int flags;
    unsigned char byte;
    unsigned short leftover;
    int s;


    s = hal_save_cli(); //sploff();                 /* disable interrupts */

    /* Put counter in count down mode */
    outb(pitctl_port, PIT_COUNTDOWN);
    /* output a count of -1 to counter 0 */
    outb(pitctr0_port, 0xff);
    outb(pitctr0_port, 0xff);
    ten_microsec_sleep_loop_count=MICROCOUNT;
    tenmicrosec();
    /* Read the value left in the counter */
    byte = inb(pitctr0_port);	/* least siginifcant */
    leftover = inb(pitctr0_port);	/* most significant */
    leftover = (leftover<<8) + byte ;
    /* Formula for delaycount is :
     *  (loopcount * timer clock speed)/ (counter ticks * 1000)
     *  Note also that 1000 is for figuring out milliseconds
     */
    ten_microsec_sleep_loop_count = (MICROCOUNT * CLKNUM) / ((0xffff-leftover)*100000);
    if (!ten_microsec_sleep_loop_count)
        ten_microsec_sleep_loop_count++;

    //splon(s);         /* restore interrupt state */
    if(s) hal_sti();
}








#define	PIT_SEL0	0x00	/* select counter 0 */
#define	PIT_SEL1	0x40	/* select counter 1 */
#define	PIT_SEL2	0x80	/* select counter 2 */

#define pit_read(n)					\
    ({	int value;					\
    outb(PITCTL, PIT_SEL##n);				\
    value = inb(PITCTR##n) & 0xff;			\
    value |= (inb(PITCTR##n) & 0xff) << 8;		\
    value;						\
    })






void phantom_timer0_start(void)
{
    //unsigned int	flags;
    unsigned char	byte;
    int s;

    findspeed();
    microfind();

    s = hal_save_cli(); 	// disable interrupts

    /* Since we use only timer 0, we program that.
     * 8254 Manual specifically says you do not need to program
     * timers you do not use
     */

    outb(pitctl_port, pit0_mode);

    unsigned int clknumb = CLKNUM/hz;

    byte = clknumb;
    outb(pitctr0_port, byte);

    byte = clknumb>>8;
    outb(pitctr0_port, byte);
    
    if(s) hal_sti();            // restore interrupt state
}



static int usec_per_tick = 10000; // 100 Hz

//static int revive_pit_int_cnt = 0;
static int revive_pit_int_cnt = 0;
static int revive_rtc_int_cnt = 0;

static void timer_int_handler()
{
#if RTC_DEBUG
putchar('#');
#endif
    revive_pit_int_cnt++;
    //putchar('|');
    hal_time_tick(usec_per_tick);
#if RTC_DEBUG
putchar('^');
#endif
}


void t0_revive(void)
{
    // Called from RTC interrupt
    revive_rtc_int_cnt++;

    if(revive_rtc_int_cnt > 300)
    {
        revive_rtc_int_cnt = 0;
        if(revive_pit_int_cnt < 2)
        {
            int m = phantom_pic_get_irqmask();
            printf("hal_time_tick locked, mask = %X\n", m );
            //phantom_timer0_start();

            // Clear lower bit
            //phantom_pic_set_irqmask(m & ~1);
        }

        revive_pit_int_cnt = 0;
    }
}



static int pit_arch_get_tick_rate(void)
{
    return usec_per_tick;
}

static bigtime_t pit_arch_get_time_delta(void)
{
    int ei = hal_save_cli();
    unsigned int d = pit_read(0);
    if (ei) hal_sti();
    
    assert(d > 0);
    if (!--d)
        return 0;

    //bigtime_t r = (bigtime_t)(CLKNUM / hz - d) * 1000000 / CLKNUM;
    bigtime_t r = ( (CLKNUM / hz - ((bigtime_t)d) ) * 1000000 ) / CLKNUM;

#if PIT_OVERFLOW_HACK
    if( r > 1000000u / hz )
    {
        int once = 1;
        //if(once)
        {
            once = 0;
            LOG_ERROR( 0, "r %lld > 1000000u / hz %u (d = %d)", r, hz, d );
            return 1000000u / hz; // TODO HACK HACK!
        }
    }
#else
    assert(r < 1000000u / hz);
#endif
    return r;
}

int
phantom_timer_pit_init(int freq, void (*timer_intr)())
{

    assert(freq == HZ);


    // Install interrupt handler. Not shareable!
    int rc = hal_irq_alloc(0, timer_int_handler, 0, 0);
    if(rc)
        panic( "phantom_timer_pit_init: can't install intr handler %x", rc);


    usec_per_tick = 1000000/hz;
    SHOW_FLOW( 1, "main timer usec_per_tick = %d", usec_per_tick );

    phantom_timer0_start();

    srandom(pit_read(0)); // try to init random gen

    arch_get_tick_rate = pit_arch_get_tick_rate;
    arch_get_time_delta = pit_arch_get_time_delta;

#if 0
    bigtime_t a = hal_system_time();
    bigtime_t start = a;

    // check that system time is monotonous
    while (a - start < 1000000)
    {
        bigtime_t b = hal_system_time();
        assert(a <= b);
        a = b;
    }
    // ...even with interrupts disabled
    int ei = hal_save_cli();
    while (a - start < 2000000)
    {
        bigtime_t b = hal_system_time();
        assert(a <= b);
        assert(a + usec_per_tick >= b);
        a = b;
    }
    if (ei) hal_sti();
#endif

    init_rtc_timer_interrupts();


    return freq;
}




/*
int
phantom_timer_pit_read()
{
    int value;

    int s = hal_save_cli();

    value = pit_read(0);

    if(s) hal_sti();
    return value;
}
*/


/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * Copyright (c) 1991 IBM Corporation
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation,
 * and that the name IBM not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written
 * prior permission.
 *
 * CARNEGIE MELLON AND IBM ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON AND IBM DISCLAIM ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 * HISTORY
 * $Log:	pit.c,v $
 * Revision 2.11  93/02/04  07:56:56  danner
 * 	Make spinwait() and tenmicrosec() [findspeed() & microtime()]
 * 	available for everyone.  They once were but they were incorrect.
 * 	Re-add tenmicrosec for PS2.
 * 	[92/02/25           dbg@ibm]
 *
 * Revision 2.9  91/10/07  17:25:03  af
 * 	tenmicrosec() was all wrong has been expunged, since noone uses
 * 	it.
 * 	[91/09/04            rvb]
 *
 * Revision 2.8  91/06/19  11:55:29  rvb
 * 	cputypes.h->platforms.h
 * 	[91/06/12  13:45:16  rvb]
 *
 * Revision 2.7  91/05/14  16:14:40  mrt
 * 	Correcting copyright
 *
 * Revision 2.6  91/02/05  17:14:03  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:37:14  mrt]
 *
 * Revision 2.5  91/01/09  19:25:33  rpd
 * 	Fixed clkstart to reset clock interrupt priority, etc.
 * 	[91/01/09            rpd]
 *
 * 	Flush dead EXL code.
 * 	[90/11/27  11:38:08  rvb]
 *
 * Revision 2.3  90/08/27  21:57:57  dbg
 * 	Fix Intel Copyright as per B. Davies authorization.
 * 	[90/08/14            dbg]
 * 	Add Intel copyright.
 * 	[90/01/08            rvb]
 * 	splall/backall -> splon/sploff
 * 	[89/10/20            rvb]
 *
 * Revision 2.2  90/05/03  15:36:34  dbg
 * 	Converted for pure kernel.
 * 	[90/02/20            dbg]
 *
 * Revision 2.2  89/09/25  12:32:40  rvb
 * 	File was provided by Intel 9/18/89.
 * 	[89/09/23            rvb]
 *
 */

/*
 Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.

 All Rights Reserved

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby
 granted, provided that the above copyright notice appears in all
 copies and that both the copyright notice and this permission notice
 appear in supporting documentation, and that the name of Intel
 not be used in advertising or publicity pertaining to distribution
 of the software without specific, written prior permission.

 INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
 CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
 NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
