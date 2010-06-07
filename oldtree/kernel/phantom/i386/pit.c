
#include <i386/pio.h>
#include <phantom_libc.h>

#include <hal.h>
#include <time.h>
#include "../misc.h"

#include "pit.h"


int	hz = HZ;		/* number of ticks per second */


int pitctl_port  = PITCTL;		/* For 386/20 Board */
int pitctr0_port = PITCTR0;	/* For 386/20 Board */
int pitctr1_port = PITCTR1;	/* For 386/20 Board */
int pitctr2_port = PITCTR2;	/* For 386/20 Board */
/* We want PIT 0 in square wave mode */

int pit0_mode = PIT_C0|PIT_NDIVMODE|PIT_READMODE ;


unsigned int delaycount;		/* loop count in trying to delay for
* 1 millisecond
*/

unsigned long ten_microsec_sleep_loop_count = 50;		/* loop count for 10 microsecond wait.
MUST be initialized for those who
insist on calling "tenmicrosec"
it before the clock has been
initialized.
*/
unsigned int clknumb = CLKNUM;		/* interrupt interval for timer 0 */

#define uWAIT 0



/* number of milliseconds to delay */
void phantom_spinwait(int millis)
{
    int i;
    unsigned int j;

    for (i=0;i<millis;i++)
        for (j=0;j<delaycount;j++)
            ;
}



#define COUNT   10000   /* should be a multiple of 1000! */

static void findspeed()
{
    //unsigned int flags;
    unsigned char byte;
    unsigned int leftover;
    //int i;
    //int j;
    int s;

    s = hal_save_cli(); //sploff();                 /* disable interrupts */

    /* Put counter in count down mode */
#define PIT_COUNTDOWN PIT_READMODE|PIT_NDIVMODE
    outb(pitctl_port, PIT_COUNTDOWN);

    /* output a count of -1 to counter 0 */
    outb(pitctr0_port, 0xff);
    outb(pitctr0_port, 0xff);
    delaycount = COUNT;
    phantom_spinwait(1);

    /* Read the value left in the counter */
    byte = inb(pitctr0_port);	/* least siginifcant */
    leftover = inb(pitctr0_port);	/* most significant */
    leftover = (leftover<<8) + byte ;

    /*
     * Formula for delaycount is :
     *  (loopcount * timer clock speed)/ (counter ticks * 1000)
     * 1000 is for figuring out milliseconds
     */

    /* we arrange calculation so that it doesn't overflow */
    delaycount = ((COUNT/1000) * CLKNUM) / (0xffff-leftover);
    printf("findspeed: delaycount=%d (tics=%d)\n", delaycount, (0xffff-leftover));
    //splon(s);         /* restore interrupt state */
    if(s) hal_sti();
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

    //intpri[0] = SPLHI;
    //form_pic_mask();

    findspeed();
//#if uWAIT
    microfind();
//#endif
    s = hal_save_cli(); //sploff();         /* disable interrupts */

    /* Since we use only timer 0, we program that.
     * 8254 Manual specifically says you do not need to program
     * timers you do not use
     */
    outb(pitctl_port, pit0_mode);
    clknumb = CLKNUM/hz;
    byte = clknumb;
    outb(pitctr0_port, byte);
    byte = clknumb>>8;
    outb(pitctr0_port, byte);
    //splon(s);         /* restore interrupt state */
    if(s) hal_sti();
}


// TODO this code must be gone in src release!
// Now replace OSENV stuff


static void(*handler)();

static int usec_per_tick = 10000; // 100 Hz

static void timer_int_handler()
{
    hal_time_tick(usec_per_tick);

    //phantom_scheduler_time_interrupt();

    //if(handler != 0) handler();
}

static int pit_arch_get_tick_rate(void)
{
    return usec_per_tick;
}

static bigtime_t pit_arch_get_time_delta(void)
{
    int ei = hal_save_cli();
    int d = pit_read(0);
    if (ei) hal_sti();
    
    assert(d > 0);
    if (!--d)
        return 0;

    bigtime_t r = (bigtime_t)(CLKNUM / hz - d) * 1000000 / CLKNUM;

    assert(r < 1000000u / hz);

    return r;
}

int
phantom_timer_pit_init(int freq, void (*timer_intr)())
{
    int rc;

    //if(freq != HZ)        panic( /*__FUNCTION__*/ "phantom_timer_pit_init: freq is %d, not %d", freq, HZ);
    assert(freq == HZ);


    // TODO why do we start first and setup intr handler after?
    phantom_timer0_start();

    usec_per_tick = 1000000/hz;
    printf("main timer usec_per_tick = %d\n", usec_per_tick);

    handler = timer_intr;

    // Install interrupt handler.
    if ((rc = hal_irq_alloc(0, timer_int_handler, 0, 0)))
    //if ((rc = hal_irq_alloc(0, timer_intr, 0, 0)))
        panic( /*__FUNCTION__*/ "phantom_timer_pit_init: can't install intr handler %x", rc);

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

    return freq;
}


/*
void
phantom_timer_pit_shutdown()
{
    osenv_irq_free(0, handler, 0);
}
*/


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
