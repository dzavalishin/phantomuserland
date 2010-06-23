#include <hal.h>
#include <i386/pio.h>
//#include <i386/isa/isa.h>
#include <x86/isa.h>
#include <phantom_libc.h>
#include <kernel/init.h>

#include <misc.h>


#include "idt.h"


// TODO get DELAY from FrteeBSD too


#define DELAY(msec) \
    do { long long dl = msec*100; \
    while( dl-- > 0 ) \
       inb(0x80);\
    } while(0);




void
hal_cpu_reset_real(void) //__attribute__((noreturn))
{
    //struct region_descriptor null_idt;
    static struct pseudo_descriptor null_pdesc;

    int b;


    //disable_intr();
    hal_cli();

    /*if (cpu == CPU_GEODE1100) {
        // Attempt Geode's own reset
        outl(0xcf8, 0x80009044ul);
        outl(0xcfc, 0xf);
    }*/

#if !defined(BROKEN_KEYBOARD_RESET)
    /*
     * Attempt to do a CPU reset via the keyboard controller,
     * do not turn off GateA20, as any machine that fails
     * to do the reset here would then end up in no man's land.
     */
    outb(IO_KBD + 4, 0xFE);
    //kb_command(KC_CMD_PULSE & ~KO_SYSRESET);
    DELAY(500000);	/* wait 0.5 sec to see if that did it */
#endif

    /*
     * Attempt to force a reset via the Reset Control register at
     * I/O port 0xcf9.  Bit 2 forces a system reset when it
     * transitions from 0 to 1.  Bit 1 selects the type of reset
     * to attempt: 0 selects a "soft" reset, and 1 selects a
     * "hard" reset.  We try a "hard" reset.  The first write sets
     * bit 1 to select a "hard" reset and clears bit 2.  The
     * second write forces a 0 -> 1 transition in bit 2 to trigger
     * a reset.
     */
    outb(((unsigned short)0xcf9u), 0x2);
    outb(((unsigned short)0xcf9u), 0x6);
    DELAY(500000);  /* wait 0.5 sec to see if that did it */

    /*
     * Attempt to force a reset via the Fast A20 and Init register
     * at I/O port 0x92.  Bit 1 serves as an alternate A20 gate.
     * Bit 0 asserts INIT# when set to 1.  We are careful to only
     * preserve bit 1 while setting bit 0.  We also must clear bit
     * 0 before setting it if it isn't already clear.
     */
    b = inb(0x92);
    if (b != 0xff) {
        if ((b & 0x1) != 0)
            outb(0x92, b & 0xfe);
        outb(0x92, b | 0x1);
        DELAY(500000);  /* wait 0.5 sec to see if that did it */
    }


    printf("No known reset method worked, attempting CPU shutdown\n");
    DELAY(1000000); /* wait 1 sec for printf to complete */

    /* Wipe the IDT. */
    set_idt(&null_pdesc);


    /* "good night, sweet prince .... <THUNK!>" */
    //breakpoint();
    asm("int $3");

    /* NOTREACHED */
    while(1);
}





// Taken from FreeBSD

/*-
 * Copyright (c) 1982, 1986 The Regents of the University of California.
 * Copyright (c) 1989, 1990 William Jolitz
 * Copyright (c) 1994 John Dyson
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, and William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)vm_machdep.c	7.3 (Berkeley) 5/13/91
 *	Utah $Hdr: vm_machdep.c 1.16.1.1 89/06/23$
 */


