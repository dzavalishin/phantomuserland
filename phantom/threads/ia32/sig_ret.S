/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Low level signal return code. Copied to user stack on signal delivery.
 *
**/

#include <mach/asm.h>

ENTRY(signal_return_code_start)

    addl	$12, %esp	// Flushes the 3 arguments to user sig handler

    popa
    ret

ENTRY(signal_return_code_end)

