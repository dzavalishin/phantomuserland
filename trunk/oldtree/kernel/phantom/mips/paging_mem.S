/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * MIPS pagetable mem.
 *
**/

#include <mips/asm.h>
#include <mips/pmap.h>
#include <kernel/page.h>

/* why define it here? c struct works too - on MIPS CPU doesn't walk pagetables

.data

    .p2align ARM_PD_SHIFT
    .globl	EXT(phantom_pdir_mem)
LEXT(phantom_pdir_mem)
    .space	ARM_PD_SIZE

    //P2ALIGN(ARM_PT_SHIFT)

    .p2align ARM_PT_SHIFT
    .globl	EXT(phantom_ptab_mem)
LEXT(phantom_ptab_mem)
    .space  ARM_PT_SIZE*NPDE

*/