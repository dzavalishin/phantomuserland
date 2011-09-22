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
 * $Log:	strcpy.c,v $
 * Revision 2.4  93/01/24  13:24:18  danner
 * 	typo
 * 	[92/10/23  10:46:33  rvb]
 * 
 * 	Created!
 * 	[92/10/22            rvb]
 * 
 *
 * File: 	libmach_sa/strcpy.c
 * Author:	Robert V. Baron  at Carnegie Mellon
 * Date:	Oct 13, 1992
 * Abstract:
 *	strcpy copies the contents of the string "from" including 
 *	the null terminator to the string "to". A pointer to "to"
 *	is returned.
 */

#include <phantom_libc.h>

char *strcpy(char *to, const char *from)
{
    register char *ret = to;

    while( (*to++ = *from++) )
        ;

    return ret;
}

#endif
