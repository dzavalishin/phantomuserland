/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * MIPS heap pool.
 *
**/

#include <kernel/config.h>
#include <kernel/page.h>

// default qemu mips machie can't accept more
#if 1
#undef PHANTOM_START_HEAP_SIZE
#define PHANTOM_START_HEAP_SIZE (10*1024*1024)
#warning Heap sz redefine
#endif

#include <mips/asm.h>

    .bss
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

