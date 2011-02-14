/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Static constructors runner.
 *
 *
**/

// TODO doesn't really run constructors :(. see multiboot.c


#include <sys/types.h>
#include <kernel/init.h>

/* These magic symbols are provided by the linker.  */
extern void (*__preinit_array_start []) (void) __attribute__((weak));
extern void (*__preinit_array_end []) (void) __attribute__((weak));
extern void (*__init_array_start []) (void) __attribute__((weak));
extern void (*__init_array_end []) (void) __attribute__((weak));
extern void (*__fini_array_start []) (void) __attribute__((weak));
extern void (*__fini_array_end []) (void) __attribute__((weak));

//extern void _init (void);
//extern void _fini (void);

/* Iterate over all the init routines.  */
void __phantom_run_constructors (void)
{
    size_t count;
    size_t i;
/*
    count = __preinit_array_end - __preinit_array_start;
    for (i = 0; i < count; i++)
        __preinit_array_start[i] ();
*/
//    _init ();
#if 1
    count = __init_array_end - __init_array_start;
    for (i = 0; i < count; i++)
        __init_array_start[i] ();
#endif
}

/* Run all the cleanup routines.  * /
void
__libc_fini_array (void)
{
    size_t count;
    size_t i;

    count = __fini_array_end - __fini_array_start;
    for (i = count; i > 0; i--)
        __fini_array_start[i-1] ();

//    _fini ();
}
*/

