/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel boot stack for e2k.
 *
**/

#include <e2k/asm.h>
#include "stack.h"

#if 0
	.bss

	P2ALIGN(PAGE_SHIFT)
	.globl	EXT(phantom_start_stack_start)
LEXT(phantom_start_stack_start)

	.space	PHANTOM_START_STACK_SIZE

	.globl	EXT(phantom_start_stack_end)
LEXT(phantom_start_stack_end)
	
	.data
	.globl	EXT(phantom_start_stack_size)
LEXT(phantom_start_stack_size)
	.long	PHANTOM_START_STACK_SIZE

#endif
