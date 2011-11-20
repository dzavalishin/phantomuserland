/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel stack for ia32. TODO move to ARCH dir!
 *
**/

#include <amd64/asm.h>
#include <stack.h>

	.bss

	.p2align PAGE_SHIFT
	.globl	EXT(phantom_start_stack_start)
EXT(phantom_start_stack_start):

	.space	PHANTOM_START_STACK_SIZE

	.globl	EXT(phantom_start_stack_end)
EXT(phantom_start_stack_end):
	
	.data
	.globl	EXT(phantom_start_stack_size)
EXT(phantom_start_stack_size):
	.long	PHANTOM_START_STACK_SIZE
