/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ARM heap pool.
 *
**/

#include <kernel/config.h>
#include <kernel/page.h>

#include <arm/asm.h>

    .bss
//     P2ALIGN(PAGE_SHIFT)
    .p2align PAGE_SHIFT

.global _C_LABEL(phantom_start_heap_start)
_C_LABEL(phantom_start_heap_start):

	.space	PHANTOM_START_HEAP_SIZE

.global _C_LABEL(phantom_start_heap_end)
_C_LABEL(phantom_start_heap_end):
	
	.data
	.global	_C_LABEL(phantom_start_heap_size)
_C_LABEL(phantom_start_heap_size):
	.long	PHANTOM_START_HEAP_SIZE

