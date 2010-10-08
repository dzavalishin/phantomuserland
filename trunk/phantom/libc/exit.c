/* 
 * Mach Operating System
 * Copyright (c) 1993-1989 Carnegie Mellon University
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
 * $Log:	exit.c,v $
 * Revision 2.6  93/01/24  13:28:01  danner
 * 	Corrected include of mach/mach.h to mach.h.
 * 	[93/01/16            mrt]
 * 
 * Revision 2.5  93/01/14  18:03:03  danner
 * 	Wrote atexit.  Not thread safe.
 * 	[92/11/05            cmaeda]
 * 
 * Revision 2.2  90/11/05  14:35:34  rpd
 * 	Created.
 * 	[90/10/30            rpd]
 * 
 */

#include <phantom_libc.h>
#include <kernel/init.h>

#if !defined(KERNEL)

/*
 * an ANSI compliant atexit function
 */

typedef void (*void_function_ptr)();
static void_function_ptr _atexit_functions[32];
static int _atexit_index = 0;

int _atexit(void (*function)())
{
     /*
      * We must support at least 32 atexit functions
      * but we don't have to support any more.
      */
     if (_atexit_index >= 32)
         return -1;

     _atexit_functions[_atexit_index++] = function;
     return 0;
}
     
int atexit(void (*function)())
{
     return _atexit(function);
}

/*
 * Call atexit functions in reverse order.
 */
static void _run_atexits(void)
{
     int i;

     for (i = _atexit_index - 1; i >= 0; i--)
         (*_atexit_functions[i])();
}
#endif


void exit(int code)
{
#if !defined(KERNEL)
    _run_atexits();
#endif
    _exit(code);
}
