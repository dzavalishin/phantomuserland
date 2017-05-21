#include <kernel/config.h>
#include <kernel/page.h>

#include <e2k/asm.h>


// moved to .c
#if 0

//    .bss
.section "bss"
    P2ALIGN(PAGE_SHIFT)

    .globl	EXT(phantom_start_heap_start)
LEXT(phantom_start_heap_start)

    .space	PHANTOM_START_HEAP_SIZE

    .globl	EXT(phantom_start_heap_end)
LEXT(phantom_start_heap_end)
	
    .data
    .globl	EXT(phantom_start_heap_size)
LEXT(phantom_start_heap_size)
    .long	PHANTOM_START_HEAP_SIZE


#endif
