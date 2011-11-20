
/* Definitions for 8254 Programmable Interrupt Timer ports on AT 386 */
#define PITCTR0		0x40		/* counter 0 port */
#define PITCTR1		0x41		/* counter 1 port */
#define PITCTR2		0x42		/* counter 2 port */
#define PITCTL		0x43		/* PIT control port */
#define PITAUX		0x61		/* PIT auxiliary port */
/* bits used in auxiliary control port for timer 2 */
#define PITAUX_GATE2	0x01		/* aux port, PIT gate 2 input */
#define PITAUX_OUT2	0x02		/* aux port, PIT clock out 2 enable */




/* Following are used for Timer 0 */
#define PIT_C0          0x00            /* select counter 0 */
#define PIT_LOADMODE	0x30		/* load least significant byte followed
					 * by most significant byte */
#define PIT_NDIVMODE	0x04		/*divide by N counter */
#define PIT_SQUAREMODE	0x06		/* square-wave mode */

/* Used for Timer 1. Used for delay calculations in countdown mode */
#define PIT_C1          0x40            /* select counter 1 */
#define PIT_READMODE	0x30		/* read or load least significant byte
					 * followed by most significant byte */
#define PIT_RATEMODE	0x06		/* square-wave mode for USART */

/*
 * Clock speed for the timer in hz divided by the constant HZ
 * (defined in param.h)
 */
//#if	AT386 || PS2
//#define CLKNUM		1193167

//#if	iPSC386
//#define CLKNUM          1000000

// dz
#define CLKNUM		1193182

#define HZ 100

#if	EXL
/* added micro-timer support.   --- csy */
typedef struct time_latch {
		time_t	ticks;          /* time in HZ since boot */
		time_t	uticks;         /* time in 1.25 MHZ */
/* don't need these two for now.   --- csy */
/*		time_t  secs;           // seconds since boot
		time_t  epochsecs;      // seconds since epoch */
	} time_latch;
/* a couple in-line assembly codes for efficiency. */
asm  int   intr_disable()
{
     pushfl
     cli
}

asm  int   intr_restore()
{
     popfl
}

#endif	// EXL



/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
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
 * $Log:	pit.h,v $
 * Revision 2.9  93/02/04  07:57:02  danner
 * 	Add PS2 to #if AT386.
 * 	[93/01/25            rvb]
 * 
 * Revision 2.8  91/06/19  11:55:33  rvb
 * 	cputypes.h->platforms.h
 * 	[91/06/12  13:45:21  rvb]
 * 
 * Revision 2.7  91/05/14  16:14:59  mrt
 * 	Correcting copyright
 * 
 * Revision 2.6  91/02/05  17:14:13  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:37:22  mrt]
 * 
 * Revision 2.5  90/12/04  14:46:28  jsb
 * 	iPSC2 -> iPSC386.
 * 	[90/12/04  11:18:09  jsb]
 * 
 * Revision 2.4  90/09/23  17:45:18  jsb
 * 	Added support for iPSC386.
 * 	[90/09/21  16:41:53  jsb]
 * 
 * Revision 2.3  90/08/27  21:58:05  dbg
 * 	Fix Intel Copyright as per B. Davies authorization.
 * 	[90/08/14            dbg]
 * 	Add Intel copyright.
 * 	[90/01/08            rvb]
 * 
 * Revision 2.2  90/05/03  15:36:57  dbg
 * 	First checkin.
 * 
 * Revision 2.2  89/09/25  12:32:44  rvb
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
