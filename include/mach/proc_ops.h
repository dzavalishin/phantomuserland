/*
 * Copyright (c) 1994 The University of Utah and
 * the Center for Software Science (CSS).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 *      Author: Bryan Ford, University of Utah CSS
 */
#ifndef _MACH_I386_PROC_OPS_H_
#define _MACH_I386_PROC_OPS_H_

#include <mach/machine/vm_types.h>
#include <mach/inline.h>

/* Returns the bit number of the most-significant set bit in `val',
   e.g. 0 for 1, 1 for 2-3, 2 for 4-7, etc.
   If `val' is 0 (i.e. no bits are set), the behavior is undefined.  */
MACH_INLINE int find_msb_set(natural_t val)
{
	int msb;

	asm("
		bsr	%0,%0
	" : "=r" (msb) : "0" (val));

	return msb;
}

#endif _MACH_I386_PROC_OPS_H_
