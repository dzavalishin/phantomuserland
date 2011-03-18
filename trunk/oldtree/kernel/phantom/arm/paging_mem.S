/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ARM pagetable mem.
 *
**/

#include "arm/asm.h"
#include <arm/pmap.h>
#include <kernel/page.h>

	.data

	P2ALIGN(ARM_PD_SHIFT)
	.globl	EXT(phantom_pdir_mem)
LEXT(phantom_pdir_mem)
	.space	ARM_PD_SIZE

	P2ALIGN(ARM_PT_SHIFT)
	.globl	EXT(phantom_ptab_mem)
LEXT(phantom_ptab_mem)
	.space  ARM_PT_SIZE*NPDE
	
