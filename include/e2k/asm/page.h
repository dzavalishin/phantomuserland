/* $Id: page.h,v 1.41 2009/07/24 12:02:54 kravtsunov_e Exp $
 *
 * Copyright 2001 Salavat S. Guiliazov (atic@mcst.ru)
 */

#ifndef _E2K_PAGE_H
#define _E2K_PAGE_H

#define	E2K_4K_PAGE_SHIFT		12		   /*  4 KBytes page */
#define	E2K_2M_PAGE_SHIFT		21		   /*  2 MBytes page */
#define	E2K_4M_PAGE_SHIFT		22		   /*  4 MBytes page */

#define	E2K_SMALL_PAGE_SHIFT		E2K_4K_PAGE_SHIFT  /*  4 KBytes page */

/* 
 * large page: 4MBytes for E3M/E2C+ and 2MBytes for others
 */

#define	E2K_LARGE_PAGE_SHIFT \
	((machine.iset_ver <= E2K_ISET_V2) ? (E2K_4M_PAGE_SHIFT) \
					   : (E2K_2M_PAGE_SHIFT))

#define	BOOT_E2K_LARGE_PAGE_SHIFT \
	((boot_machine.iset_ver <= E2K_ISET_V2) ? (E2K_4M_PAGE_SHIFT) \
						: (E2K_2M_PAGE_SHIFT))

/*  4 KBytes page */
#define	PAGE_SHIFT			E2K_SMALL_PAGE_SHIFT

/*  large page    */
#define	LARGE_PAGE_SHIFT		E2K_LARGE_PAGE_SHIFT

#ifndef __ASSEMBLY__
#define	E2K_SMALL_PAGE_SIZE		(1UL << E2K_SMALL_PAGE_SHIFT)

#define	E2K_4M_PAGE_SIZE		(1UL << E2K_4M_PAGE_SHIFT)

#define	E2K_LARGE_PAGE_SIZE		(1UL << E2K_LARGE_PAGE_SHIFT)
#define	BOOT_E2K_LARGE_PAGE_SIZE	(1UL << BOOT_E2K_LARGE_PAGE_SHIFT)

#define PAGE_SIZE    			(1UL << PAGE_SHIFT)
#define LARGE_PAGE_SIZE    		E2K_LARGE_PAGE_SIZE
#else	/* __ASSEMBLY__ */
#define	E2K_SMALL_PAGE_SIZE		(1 << E2K_SMALL_PAGE_SHIFT)

#define	E2K_LARGE_PAGE_SIZE		(1 << E2K_LARGE_PAGE_SHIFT)
#define	BOOT_E2K_LARGE_PAGE_SIZE	(1 << BOOT_E2K_LARGE_PAGE_SHIFT)

#define PAGE_SIZE    			(1 << PAGE_SHIFT)
#define LARGE_PAGE_SIZE    		E2K_LARGE_PAGE_SIZE
#endif	/* !(__ASSEMBLY__) */

#define E2K_MAX_PAGE_SIZE		(1 << E2K_4M_PAGE_SHIFT)

#define	E2K_SMALL_PAGE_MASK		(~(E2K_SMALL_PAGE_SIZE - 1))

#define	E2K_LARGE_PAGE_MASK		(~(E2K_LARGE_PAGE_SIZE - 1))
#define	BOOT_E2K_LARGE_PAGE_MASK	(~(BOOT_E2K_LARGE_PAGE_SIZE - 1))

#define	PAGE_MASK			(~(PAGE_SIZE - 1))
#define	LARGE_PAGE_MASK			E2K_LARGE_PAGE_MASK

#define HPAGE_SHIFT			E2K_LARGE_PAGE_SHIFT
#define HPAGE_SIZE			((1UL) << HPAGE_SHIFT)
#define HPAGE_MASK			(~(HPAGE_SIZE - 1))
#define HUGETLB_PAGE_ORDER		(HPAGE_SHIFT - PAGE_SHIFT)

#define HPAGE_PMD_MAX_ORDER		(E2K_4M_PAGE_SHIFT - PAGE_SHIFT)


#ifdef __KERNEL__

#include <linux/init.h>

#include <asm/types.h>
#include <asm/errors_hndl.h>
#ifndef __ASSEMBLY__
#include <asm/atomic.h>
#include <asm/e2k_api.h>
#include <asm/bootinfo.h>
#endif	/* !(__ASSEMBLY__) */


/*
 * To align the pointer to the (next) page boundary or to the beginning of
 * the page
 */

/*
 * Round up the beginning of the address.
 */
#define	_PAGE_ALIGN_UP(addr, page_size) \
		((e2k_addr_t)(addr) & ~(page_size-1))

/*
 * Round down the end of the address.
 */
#define	_PAGE_ALIGN_DOWN(addr, page_size) \
		(((e2k_addr_t)(addr) + (page_size-1)) & ~(page_size-1))

#define	E2K_SMALL_PAGE_ALIGN_DOWN(addr) \
		_PAGE_ALIGN_DOWN(addr, E2K_SMALL_PAGE_SIZE)
#define	E2K_SMALL_PAGE_ALIGN_UP(addr) \
		_PAGE_ALIGN_UP(addr, E2K_SMALL_PAGE_SIZE)
#define	E2K_SMALL_PAGE_ALIGN(addr)	E2K_SMALL_PAGE_ALIGN_DOWN(addr)

#define	E2K_LARGE_PAGE_ALIGN_DOWN(addr) \
		_PAGE_ALIGN_DOWN(addr, E2K_LARGE_PAGE_SIZE)
#define	E2K_LARGE_PAGE_ALIGN_UP(addr) \
		_PAGE_ALIGN_UP(addr, E2K_LARGE_PAGE_SIZE)
#define	E2K_LARGE_PAGE_ALIGN(addr)	E2K_LARGE_PAGE_ALIGN_DOWN(addr)


#define	PAGE_ALIGN_DOWN(addr)		_PAGE_ALIGN_DOWN(addr, PAGE_SIZE)
#define	PAGE_ALIGN_UP(addr)		_PAGE_ALIGN_UP(addr, PAGE_SIZE)
#define	LARGE_PAGE_ALIGN_DOWN(addr)	_PAGE_ALIGN_DOWN(addr, LARGE_PAGE_SIZE)
#define	LARGE_PAGE_ALIGN_UP(addr)	_PAGE_ALIGN_UP(addr, LARGE_PAGE_SIZE)

#define	ALIGN_MASK_UP(addr, mask)	((addr) & ~(mask))
#define	ALIGN_MASK_DOWN(addr, mask)	(((addr) + (mask)) & ~(mask))
#define	ALIGN_MASK(addr, mask)		ALIGN_MASK_DOWN(addr, mask)

#ifndef __ASSEMBLY__

/*
 * A _lot_ of the kernel time is spent clearing pages, so
 * do this as fast as it possibly can.
 *
 * Using MAS_BYPASS_ALL_CACHES allows us to avoid caches pollution.
 * If a cleared area is bigger than write buffers size (1024 bytes
 * on E3M, E3S, E2S, E8C, E1C+), then this becomes much slower than disabling
 * only L1 cache even with write-combining, but caches on e2k are
 * small and we must spare them.
 */
#define clear_memory_8(addr, size) recovery_memset_8(addr, 0, ETAGEWD, size, \
		LDST_DWORD_FMT << LDST_REC_OPC_FMT_SHIFT \
		| MAS_BYPASS_ALL_CACHES << LDST_REC_OPC_MAS_SHIFT)
#define clear_page(page) recovery_memset_8(page, 0, ETAGEWD, PAGE_SIZE, \
		LDST_DWORD_FMT << LDST_REC_OPC_FMT_SHIFT \
		| MAS_BYPASS_ALL_CACHES << LDST_REC_OPC_MAS_SHIFT)
#define clear_user_page(addr, vaddr, page)	clear_page(addr)
#define clear_user_highpage(page, vaddr) \
		clear_page(page_address(page))


#define __HAVE_ARCH_COPY_USER_HIGHPAGE
#define copy_user_highpage(to, from, vaddr, vma) \
		copy_page(page_address(to), page_address(from))

#define copy_tagged_page(to, from)	_tagged_memcpy_8(to, from, PAGE_SIZE)

#define copy_page(to, from)			copy_tagged_page(to, from)
#define copy_user_page(to, from, vaddr, page)	copy_page(to, from)

/*
 * These are used to make use of C type-checking..
 */
typedef unsigned long	pteval_t;
typedef unsigned long	pmdval_t;
typedef unsigned long	pudval_t;
typedef unsigned long	pgdval_t;
typedef unsigned long	pgprotval_t;

typedef struct { pteval_t pte; } pte_t;
typedef struct { pmdval_t pmd; } pmd_t;
typedef struct { pudval_t pud; } pud_t;
typedef struct { pgdval_t pgd; } pgd_t;
typedef struct { pgprotval_t pgprot; } pgprot_t;

#define pte_val(x)	((x).pte)
#define pmd_val(x)	((x).pmd)
#define pud_val(x)	((x).pud)
#define pgd_val(x)	((x).pgd)
#define pgprot_val(x)	((x).pgprot)

#define __pte(x)	((pte_t) { (x) } )
#define __pmd(x)	((pmd_t) { (x) } )
#define __pud(x)	((pud_t) { (x) } )
#define __pgd(x)	((pgd_t) { (x) } )
#define __pgprot(x)	((pgprot_t) { (x) } )

typedef struct page *pgtable_t;

#define PAGE_OFFSET		0x0000d00000000000

#define __pa(x)			((e2k_addr_t)(x) - PAGE_OFFSET)
#define __va(x)			((void *)((e2k_addr_t) (x) + PAGE_OFFSET))

/*
 * PFNs are real physical page numbers.  However, mem_map only begins to record
 * per-page information starting at pfn_base.
 * This is to handle systems where the first physical page in the machine
 * is not 0.
 */

struct page;

#ifdef CONFIG_FLATMEM
extern unsigned long		pfn_base;
#define ARCH_PFN_OFFSET		(pfn_base)
#endif	/* CONFIG_FLATMEM */
extern struct page *e2k_virt_to_page(const void *kaddr);
#define phys_to_page(kaddr)	pfn_to_page((kaddr) >> PAGE_SHIFT)
#define virt_to_page(kaddr)						\
		(((e2k_addr_t)(kaddr) >= PAGE_OFFSET && 		\
			(e2k_addr_t)(kaddr) < PAGE_OFFSET + MAX_PM_SIZE) ? \
				phys_to_page(__pa(kaddr))		\
				:					\
				e2k_virt_to_page((void *) (kaddr)))
#define page_to_phys(page)	(page_to_pfn(page) << PAGE_SHIFT)

#define virt_to_phys		__pa
#define phys_to_virt		__va
extern int e2k_is_pfn_valid(e2k_size_t pfn);

#define	pfn_valid(pfn)		e2k_is_pfn_valid(pfn)
#define	page_valid(page)	pfn_valid(page_to_pfn(page))
#define phys_addr_valid(addr)	pfn_valid((addr) >> PAGE_SHIFT)
#define virt_addr_valid(kaddr)	page_valid(virt_to_page(kaddr))
#define kern_addr_valid(kaddr)	page_valid(virt_to_page(kaddr))

#ifndef	CONFIG_RECOVERY
#define	boot_pa(x)	(x)
#define	boot_va(x)	(x)
#else
#define	boot_pa(x)	((READ_OSCUD_LO_REG().OSCUD_lo_base >= PAGE_OFFSET) ? \
				__pa(x) : (void *)(x))
#define	boot_va(x)	((READ_OSCUD_LO_REG().OSCUD_lo_base >= PAGE_OFFSET) ? \
				__va(x) : (void *)(x))
#endif	/* ! (CONFIG_RECOVERY) */

/*
 * E2K architecture additon vm_flags
 */

#define VM_HW_STACK_PS		0x0100000000UL	/* Procedure stack area */
#define	VM_TAGMAPPED		0x0200000000UL	/* The tags area appropriate */
						/* to this data VM area was */
						/* mapped */
#define VM_HW_STACK_PCS		0x0400000000UL	/* Chain stack area */
#define	VM_WRITECOMBINED	0x0800000000UL
#define VM_DONTMIGRATE		0x1000000000UL	/* Do not migrate vma on NUMA */
#define VM_HW_STACK		0x2000000000UL	/* The resident window of */
						/* hardware stacks */
#define VM_PRIVILEGED		0x4000000000UL	/* Pages are privileged */

#define VM_CUI			0xffff000000000000UL /* CUI for pages in VMA */
#define VM_CUI_SHIFT		48

/*
 * We don't set the valid bit for PROT_NONE areas, otherwise
 * semi-speculative loads will cause page faults which is bad
 * for performance when such loads come from an unrolled loop.
 */
#define	VM_PAGESVALID		(VM_READ | VM_WRITE | VM_EXEC)

/*
 * The following structure is used to hold the physical memory configuration
 * of the machine.  This is filled in 'boot_probe_memory()' and is later
 * used by 'boot_mem_init()' to setup boot-time memory map and by 'mem_init()'
 * to set up 'mem_map[]'.
 * A base address of a bank should be page aligned.
 * The structure item 'mem_bitmap' is a map pointer. The map bits represent
 * the physical memory on the bank in terms of small pages (4 KB).
 * To reduce the boot-time map size, the boot map represents only needed
 * to boot tasks first 'BOOT_MAX_PHYS_MEM_SIZE' bytes of real physical memory
 * configuration. Some of memory areas are prereserved ('text' & 'data'
 * segments, stack, boot information etc) and have been allocated by BIOS or
 * boot loaders. All these areas are known and listed in the header
 * 'asm/boot_init.h' Such area can be allocated on any physical address and
 * can be out of the boot map, which represents reserved memory
 * The structure 'e2k_busy_mem_t' represents occupied memory areas in a bank,
 * which can not be described by the boot map.
 * Array of 'E2K_MAX_PHYS_BANKS' of these structures is statically allocated
 * into the kernel image.
 * The entry after the last valid one has 'pages_num == 0'.
 */

typedef	unsigned long	e2k_mem_map_t;	/* double-word (64 bits == 64 pages) */

typedef	struct e2k_busy_mem {
	e2k_size_t	start_page;	/* start page # of an area in a bank */
	e2k_size_t	pages_num;	/* number of occupied pages by the */
					/* area in the bank */
} e2k_busy_mem_t;

#define	E2K_MAX_PRERESERVED_AREAS	10

typedef struct e2k_phys_bank {
	e2k_addr_t	base_addr;	/* base physical address of the start */
					/* page of the bank */
	e2k_size_t	pages_num;	/* total number of pages in the bank */
	e2k_size_t	bitmap_size;	/* number of pages including in the */
					/* bitmap, can be <= 'pages_num' */
	e2k_mem_map_t	*mem_bitmap;	/* map pointer - the bits represent */
					/* all physical memory pages */
	atomic_t	free_pages_num;	/* current number of free pages */
					/* in the map */
	e2k_size_t	first_free_page;/* the first free page number in the */
					/* bank - start point to allocations */
	int		busy_areas_num;	/* number of areas in the list of */
					/* occupied areas in the bank */
	e2k_busy_mem_t	busy_areas[E2K_MAX_PRERESERVED_AREAS];
					/* list of all occupied areas in the */
					/* bank, which not included to the */
					/* memory bitmap */
	
} e2k_phys_bank_t;

typedef struct node_phys_mem {
	e2k_size_t	start_pfn;	/* start page number on the node */
	e2k_size_t	pfns_num;	/* number of pages on the node */
					/* including holes between banks */
	e2k_phys_bank_t	banks[L_MAX_NODE_PHYS_BANKS];
} node_phys_mem_t;

extern  node_phys_mem_t	nodes_phys_mem[L_MAX_MEM_NUMNODES];

#define boot_nodes_phys_mem     \
		((node_phys_mem_t *)boot_vp_to_pp(nodes_phys_mem))

extern void __init calculate_zone_sizes(e2k_phys_bank_t *node_phys_banks,
			unsigned long zone_start_pfn,
			unsigned long zone_end_pfn,
			unsigned long *zones_size, unsigned long *zholes_size,
			unsigned long *phys_lo_bound,
			unsigned long *phys_hi_bound);
extern void __meminit memmap_init_node(unsigned long size, int nid,
			node_phys_mem_t *node_mem,
			unsigned long zone, unsigned long start_pfn);
#define __HAVE_ARCH_MEMMAP_INIT
extern void __meminit memmap_init(unsigned long size, int nid, unsigned long zone,
			unsigned long start_pfn);

#define VM_DATA_DEFAULT_FLAGS	(VM_READ | VM_WRITE | \
				 VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC)

/* gdb relies on user stack being executable to implement call()
 * functionality (it sets a trampoline in traced process's stack
 * and then changes cr0.hi value accordingly). */
#define VM_STACK_DEFAULT_FLAGS \
({ \
	unsigned long __stack_def_flags = VM_DATA_DEFAULT_FLAGS; \
	if (current->ptrace & PT_PTRACED) { \
		__stack_def_flags |= VM_EXEC; \
		if (current->thread.flags & E2K_FLAG_32BIT) \
			__stack_def_flags |= \
					USER_CODES_32_INDEX << VM_CUI_SHIFT; \
		else \
			__stack_def_flags |= \
					USER_CODES_START_INDEX << VM_CUI_SHIFT;\
	} \
	__stack_def_flags; \
})

#ifdef CONFIG_SECONDARY_SPACE_SUPPORT

/*
 * Most important i386 definitions
 */ 
#define i386_PAGE_SHIFT		12
#define i386_PGDIR_SHIFT	22
#define i386_PAGE_SIZE		(1UL << i386_PAGE_SHIFT)
#define i386_PGDIR_SIZE		(1UL << i386_PGDIR_SHIFT)
#define i386_PAGE_MASK		(~(i386_PAGE_SIZE-1))
#define i386_PTE_MASK		i386_PAGE_MASK
#define i386_PTRS_PER_PGD	1024
#define i386_PTRS_PER_PTE	1024

#define i386_PAGE_PRESENT	0x001
#define i386_PAGE_RW		0x002
#define i386_PAGE_USER		0x004
#define i386_PAGE_PWT		0x008
#define i386_PAGE_PCD		0x010
#define i386_PAGE_ACCESSED	0x020
#define i386_PAGE_DIRTY		0x040
#define i386_PAGE_PSE		0x080	/* 4 MB (or 2MB) page */
#define i386_PAGE_GLOBAL	0x100	/* Global TLB entry PPro+ */

#define i386_PAGE_TABLE		(i386_PAGE_PRESENT | i386_PAGE_RW | \
					i386_PAGE_USER | i386_PAGE_ACCESSED | \
					i386_PAGE_DIRTY)
#define i386_KERNPG_TABLE	(i386_PAGE_PRESENT | i386_PAGE_RW | \
					i386_PAGE_ACCESSED | i386_PAGE_DIRTY)

typedef struct { u32 pte_low; } i386_pte_t;
typedef struct { u32 pgd; }     i386_pgd_t;
typedef struct { u32 pgprot; }  i386_pgprot_t;

#define i386_pte_val(x)		((x).pte_low)
#define i386_pgd_val(x)		((x).pgd)
#define i386_set_pte(pteptr, v)	(*(pteptr) = v)
#define i386_set_pgd(pgdptr, v)	(*(pgdptr) = v)

#define i386__pte(x)		((i386_pte_t) { (x) } )
#define i386__pgd(x)		((i386_pgd_t) { (x) } )
#define i386_pte_index(a) 	(((a) >> i386_PAGE_SHIFT) \
                 			& (i386_PTRS_PER_PTE - 1))

#define i386_pgd_index(a)	(((a) >> i386_PGDIR_SHIFT) \
					& (i386_PTRS_PER_PGD-1))
#define i386_pte_offset_kernel(dir, a) \
        ((i386_pte_t *) i386_pmd_page_kernel(*(dir)) +  i386_pte_index(a))

#define i386_pmd_page_kernel(dir) \
( __va(i386_pgd_val(dir) & i386_PAGE_MASK))

#define i386__PTE_INIT_VAL	i386__pte(i386_PAGE_RW|i386_PAGE_USER)

#endif /* CONFIG_SECONDARY_SPACE_SUPPORT */

#ifndef CONFIG_E2K_HAS_OPT_BITOPS
#include <asm-generic/getorder.h>
#else
#include <asm/bitsperlong.h>
static inline int get_order(unsigned long size)
{
	int lz = 0;

	size = (size - 1) >> PAGE_SHIFT;
	lz = E2K_LZCNTD(size);

	return BITS_PER_LONG - lz;
}
#endif /* CONFIG_E2K_HAS_OPT_BITOPS */

struct task_struct;

extern e2k_addr_t node_kernel_address_to_phys(int node, e2k_addr_t address);
extern e2k_addr_t user_address_to_phys(struct task_struct *tsk,
		e2k_addr_t address);
extern e2k_addr_t kernel_address_to_phys(e2k_addr_t address);

#endif /* !(__ASSEMBLY__) */

#endif /* !(__KERNEL__) */

#ifndef	CONFIG_ARCH_DISCONTIG_NODE_MEM_MAP
#include <asm-generic/memory_model.h>
#endif	/* ! CONFIG_ARCH_DISCONTIG_NODE_MEM_MAP */

#endif /* !(_E2K_PAGE_H) */
