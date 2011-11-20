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
 * $Log:	strcmp.c,v $
 * Revision 2.4  93/01/24  13:24:14  danner
 * 	Created!
 * 	[92/10/22            rvb]
 * 
 *
 * File: 	limach/strcmp.c
 * Author:	Robert V. Baron at Carnegie Mellon
 * Date:	Oct 13, 1992
 * Abstract:
 *	strcmp (s1, s2) compares the strings "s1" and "s2".
 *	It returns 0 if the strings are identical. It returns
 *	> 0 if the first character that differs into two strings
 *	is larger in s1 than in s2 or if s1 is longer than s2 and 
 *	the contents  are identical up to the length of s2.
 *	It returns < 0 if the first differing character is smaller 
 *	in s1 than in s2 or if s1 is shorter than s2 and the
 *	contents are identical upto the length of s1.
 */

#include <phantom_libc.h>

int
strcmp(const char *s1, const char *s2)
{
register int a, b;



	while ( (a = *s1++), (b = *s2++), a && b) {
		if (a != b)
			return (a-b);
	}

	return a-b;
}

#endif
