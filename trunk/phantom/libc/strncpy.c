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
 * $Log:	strncpy.c,v $
 * Revision 2.4  93/01/24  13:24:30  danner
 * 	Created!
 * 	[92/10/22            mrt]
 * 
 *
 * File: 	libmach_sa/strncpy.c
 * Author:	Mary R. Thompson at Carnegie Mellon
 * Date:	Oct 13, 1992
 * Abstract:
 *	strncpy copies "count" characters from the "from" string to
 *	the "to" string. If "from" contains less than "count" characters
 *	"to" will be padded with null characters until exactly "count"
 *	characters have been written. The return value is a pointer
 *	to the "to" string.
 */

#include <phantom_libc.h>

// size_t is correct, but kernel dies!
//char *	strncpy(char *to, const char *from, size_t count);
char *strncpy(char *to, const char *from, ssize_t count)
{
    register char *ret = to;

    while (count-- > 0 && (*to++ = *from++));

    while (count-- > 0) 
	*to++ = '\0';

    return ret;
}
