/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)assert.h	8.2 (Berkeley) 1/21/94
 * $FreeBSD: src/include/assert.h,v 1.4.32.1 2008/11/25 02:59:29 kensmith Exp $
 */

#include <sys/cdefs.h>

#ifndef __PANIC_DEFINED
#define __PANIC_DEFINED
// coverity[+kill]
void panic(const char *__format, ...) __dead2;
#endif // __PANIC_DEFINED


void stack_dump(void);
void stack_dump_from(void *ebp);
void *arch_get_frame_pointer();


extern char * (*phantom_symtab_getname)( void *addr );



/*
 * Unlike other ANSI header files, <assert.h> may usefully be included
 * multiple times, with and without NDEBUG defined.
 */

#undef assert
#undef _assert

#ifdef NDEBUG
#define	assert(e)	((void)0)
#define	_assert(e)	((void)0)
#else
#define	_assert(e)	assert(e)

//#define	assert(e)	((e) ? (void)0 : __assert(__func__, __FILE__, __LINE__, #e))
#define	assert(e)	((e) ? (void)0 : panic( __FILE__ ":%u, %s: assertion '" #e "' failed" , __LINE__, __func__ ))
#endif /* NDEBUG */

//__BEGIN_DECLS
//void __assert(const char *, const char *, int, const char *);
//__END_DECLS

// two top bits are 'no softint req' and 'softint disabled'
extern int      irq_nest; 


#define assert_not_interrupt() assert(!(irq_nest & ~0xC0000000))

#define assert_int_disabled() assert(!hal_is_sti())
#define assert_interrupts_disabled() assert_int_disabled()

#define assert_int_enabled() assert(hal_is_sti())
#define assert_interrupts_enabled() assert_int_enabled()
