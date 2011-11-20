#ifndef	_PHANTOM_PMAP_MACHINE_
#define _PHANTOM_PMAP_MACHINE_

#ifndef ARCH_ia32
#warning Intel32 code! Wrong arch?
#endif


#ifndef ASSEMBLER
#include <phantom_types.h>
#endif

#define NPDE 1024
#define NPTE 1024

// for physalloc - max pages to alloc
#define PHYSALLOC_MAXPAGES (NPDE*NPTE)

/*
 *	Hardware pte bit definitions (to be used directly on the ptes
 *	without using the bit fields).
 */


#define INTEL_PTE_VALID		0x00000001
#define INTEL_PTE_WRITE		0x00000002
#define INTEL_PTE_USER		0x00000004
#define INTEL_PTE_WTHRU		0x00000008
#define INTEL_PTE_NCACHE 	0x00000010
#define INTEL_PTE_REF		0x00000020
#define INTEL_PTE_MOD		0x00000040

#define INTEL_PTE_GLOBAL	0x00000100
//#define INTEL_PTE_WIRED		0x00000200
//#define INTEL_PDPTE_NESTED	0x00000400

#define INTEL_PTE_AVAIL		0x00000e00

#define INTEL_PTE_PFN		0xfffff000



#define INTEL_PDE_VALID		0x00000001
#define INTEL_PDE_WRITE		0x00000002
#define INTEL_PDE_USER		0x00000004
#define INTEL_PDE_WTHRU		0x00000008
#define INTEL_PDE_NCACHE 	0x00000010
#define INTEL_PDE_REF		0x00000020

#define INTEL_PDE_AVL		0x00000040
#define INTEL_PDE_PGSZ		0x00000080
#define INTEL_PDE_GLOBL		0x00000100

#define INTEL_PDE_PFN		0xfffff000


/*
 *	Macros to translate between page table entry values
 *	and physical addresses.
 */

#define	pa_to_pte(a)		((a) & INTEL_PTE_PFN)
#define	pte_to_pa(p)		((p) & INTEL_PTE_PFN)
#define	pte_increment_pa(p)	((p) += INTEL_OFFMASK+1)

#define	pa_to_pde(a)		((a) & INTEL_PDE_PFN)


// Combine address and mode bits into a PTE.
#define create_pte(a, m)	((pt_entry_t) (((physaddr_t) (a)) | (m)))




//#define INTEL_OFFMASK	0xfff	/* offset within page - gone to arch_page.h */
#define PDESHIFT	22	/* page descriptor shift */
#define PDEMASK		0x3ff	/* mask for page descriptor index */
#define PTESHIFT	12	/* page table shift */
#define PTEMASK		0x3ff	/* mask for page table index */





#ifndef ASSEMBLER


/*
 *	i386/i486 Page Table Entry
 */

typedef unsigned int	pt_entry_t;
typedef unsigned int	pd_entry_t;





/*
 *	Convert linear offset to page descriptor/page table index
 */
#define lin2pdenum(a)	(((a) >> PDESHIFT) & PDEMASK)
#define lin2ptenum(a)	(((a) >> PTESHIFT) & PTEMASK)

/*
 *	Convert page descriptor/page table index to linear address
 */
#define pdenum2lin(a)	((vm_offset_t)(a) << PDESHIFT)
#define ptenum2lin(a)	((vm_offset_t)(a) << PTESHIFT)







void phantom_paging_init(void);
void phantom_paging_start(void); // For SMP - to load pdir in next CPUs


void phantom_map_page(linaddr_t la, pt_entry_t mapping );
void phantom_unmap_page(linaddr_t la );

int phantom_is_page_accessed(linaddr_t la);
int phantom_is_page_modified(linaddr_t la);


#endif // ASSEMBLER




#endif	/* _PMAP_MACHINE_ */


/*
 * This header partially extracted from:
 *
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
