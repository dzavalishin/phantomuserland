// We have asm version for this arch
#ifndef ARCH_ia32

/* 
 * Mach Operating System
 * Copyright (c) 1992,1991,1990,1989 Carnegie Mellon University
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
 * $Log:	strcat.c,v $
 * Revision 2.4  93/01/24  13:24:09  danner
 * 	Created!
 * 	[92/10/22            rvb]
 * 
 *
 * File: 	libmach_sa/strcat.c
 * Author:	Robert Baron at Carnegie Mellon
 * Date:	Oct 13, 1992
 * Abstract:
 *	strcat appends the contents of "add" to the end of
 *	"s". It returns a pointer to "s".
 */

#include <phantom_libc.h>

char *
strcat(char *s, const char *add)
{
register char *ret = s;

	while(*s) s++;

	while((*s++ = *add++)) ;

	return ret;
}

#endif
