#include "mach/asm.h"
#include <kernel/page.h>

//#include <x86/phantom_page.h>
#include <x86/phantom_pmap.h>

	.data

	P2ALIGN(PAGE_SHIFT)
	.globl	EXT(phantom_pdir_mem)
LEXT(phantom_pdir_mem)

	.space	PAGE_SIZE

	P2ALIGN(PAGE_SHIFT)
	.globl	EXT(phantom_ptab_mem)
LEXT(phantom_ptab_mem)

	.space  PAGE_SIZE*NPDE
	
