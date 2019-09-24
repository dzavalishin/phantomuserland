#include <ia32/asm.h>
#include <kernel/page.h>
#include <ia32/phantom_pmap.h>
#include <kernel/config.h>

	.data

#if CONF_DUAL_PAGEMAP

	P2ALIGN(PAGE_SHIFT)
	.globl	EXT(phantom_pdir_on_mem)
LEXT(phantom_pdir_on_mem)

	.space	PAGE_SIZE

	P2ALIGN(PAGE_SHIFT)
	.globl	EXT(phantom_pdir_off_mem)
LEXT(phantom_pdir_off_mem)

	.space	PAGE_SIZE


#else
	P2ALIGN(PAGE_SHIFT)
	.globl	EXT(phantom_pdir_mem)
LEXT(phantom_pdir_mem)

	.space	PAGE_SIZE
#endif
	P2ALIGN(PAGE_SHIFT)
	.globl	EXT(phantom_ptab_mem)
LEXT(phantom_ptab_mem)

	.space  PAGE_SIZE*NPDE
	
