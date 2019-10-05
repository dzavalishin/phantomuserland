/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Panic.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include <assert.h>
#include <stdarg.h>
#include <hal.h>
#include <phantom_libc.h>

#include <thread_private.h>

#include <phantom_libc.h>

#include <kernel/board.h>
#include <kernel/boot.h>


int panic_reenter = 0;

// coverity[+kill]
void panic(const char *fmt, ...)
{
    if(panic_reenter)
        _exit(33);

    board_panic_stop_world();

    hal_cli();
    panic_reenter++;

    // CI: this word is being watched by CI scripts. Do not change -- or change CI appropriately
    printf("Panic: ");
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    if (!bootflag_unattended)
    {
	// CI: this word is being watched by CI scripts. Do not change -- or change CI appropriately
	printf("\nPress any key ...\n");
	board_panic_wait_keypress();
    }

    printf("\r             \r");

    stack_dump();

    dump_thread_stacks();

    if (!bootflag_unattended)
    {
	// CI: this word is being watched by CI scripts. Do not change -- or change CI appropriately
	printf("\nPress any key to reboot ...\n");
	board_panic_wait_keypress();
    }

    exit(33);
}


