#include "amd64/asm.h"
#include <kernel/page.h>

#include <x86/phantom_pmap.h>

	.data

	.p2align PAGE_SHIFT
	.globl	EXT(phantom_pdir_mem)
EXT(phantom_pdir_mem):

	.space	PAGE_SIZE

	.p2align PAGE_SHIFT
	.globl	EXT(phantom_ptab_mem)
EXT(phantom_ptab_mem):

	.space  PAGE_SIZE*NPDE
	
