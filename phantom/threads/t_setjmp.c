#if 0 // can't wrap with func - stack will be corrupt 
/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Machine independent part of setjmp/longjmp.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#define DEBUG_MSG_PREFIX "setjmp"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10


#include <setjmp.h>
#include <phantom_assert.h>
#include <threads.h>


int setjmp(jmp_buf j) // __attribute__((returns_twice))
{
    int tid = get_current_tid();
    int rv = setjmp_machdep(j);

    int new_tid = get_current_tid();
    if( tid != new_tid )
        panic("Cross-thread longjmp, saved state in tid %d, jump from tid %d", tid, new_tid );

    return rv;
}

void longjmp(jmp_buf j, int v)
{
    longjmp_machdep(j,v);
}

#endif

