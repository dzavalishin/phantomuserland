/* $Id: head.h,v 1.41 2009/10/27 10:14:51 atic Exp $ */
#ifndef _E2K_HEAD_H
#define	_E2K_HEAD_H

#include <asm/types.h>
#include <asm/page.h>
#include <asm/errors_hndl.h>
#include <asm/e2k_api.h>
#ifndef __ASSEMBLY__
#include <asm/sections.h>
#endif /* _ASSEMBLY__ */

#define	EOS_RAM_BASE_LABEL	_data
#define	KERNEL_START_LABEL	_start		/* start label of Linux Image */
#define	KERNEL_END_LABEL	_end		/* end label of Linux Image */

#ifdef __ASSEMBLY__

#define	KERNEL_BASE	[KERNEL_START_LABEL]	/* virtual address of Linux */
						/* Image begining */
#define	KERNEL_END	[KERNEL_END_LABEL]	/* virtual address of Linux */
						/* Image end */
#define	EOS_RAM_BASE	[EOS_RAM_BASE_LABEL]

#else /* !(__ASSEMBLY__) */

#define	EOS_RAM_BASE	((e2k_addr_t)&EOS_RAM_BASE_LABEL)

#define	KERNEL_BASE	((e2k_addr_t)&KERNEL_START_LABEL)
#define	KERNEL_END	((e2k_addr_t)&KERNEL_END_LABEL)

#endif /* !(__ASSEMBLY__) */


#define	E2K_EOS_RAM_PAGE_SIZE	E2K_SMALL_PAGE_SIZE	/* Loader warks into */
							/* the small pages */

/* Size of pages where the kernel is loaded */
#define	E2K_KERNEL_PAGE_SIZE		(cpu_has(CPU_HWBUG_LARGE_PAGES) ? \
				E2K_SMALL_PAGE_SIZE : E2K_LARGE_PAGE_SIZE)
#define	BOOT_E2K_KERNEL_PAGE_SIZE	(boot_cpu_has(CPU_HWBUG_LARGE_PAGES) ? \
				E2K_SMALL_PAGE_SIZE : BOOT_E2K_LARGE_PAGE_SIZE)

#ifdef	CONFIG_KERNEL_CODE_CONTEXT
#define	E2K_KERNEL_PROT_PAGE_SIZE	E2K_SMALL_PAGE_SIZE
							/* protected kernel */
							/* image loads into */
							/* the small pages */
#endif	/* CONFIG_KERNEL_CODE_CONTEXT */

#define	E2K_KERNEL_PS_PAGE_SIZE	E2K_SMALL_PAGE_SIZE	/* kernel procedure */
							/* stack loads into */
							/* the small pages */

							/* kernel procedure */
							/* stack size 8 * 4KB */
							/* at boot-time */
#define	E2K_BOOT_KERNEL_PS_SIZE		(16 * E2K_KERNEL_PS_PAGE_SIZE)

							/* kernel procedure */
							/* chain stack loads */
							/* into the small */
							/* pages */
#define	E2K_KERNEL_PCS_PAGE_SIZE	E2K_SMALL_PAGE_SIZE

							/* kernel procedure */
							/* chain stack size */
							/* at boot-time */
							/* 4 * 4KB */
#define	E2K_BOOT_KERNEL_PCS_SIZE	(4 * E2K_KERNEL_PCS_PAGE_SIZE)

							/* kernel stack loads */
							/* into the small */
							/* pages */
#define	E2K_KERNEL_US_PAGE_SIZE		E2K_SMALL_PAGE_SIZE

							/* kernel stack size */
							/* at boot-time */
							/* 8 * 4KB */
#define	E2K_BOOT_KERNEL_US_SIZE		(4 * E2K_KERNEL_US_PAGE_SIZE)

#define	E2K_BOOT_INIT_PSL		0		/* the level of */
							/* kernel boot start */
							/* function */
#define	E2K_BOOT_INIT_STACK_ZIP		16		/* zip area before */
							/* stack of kernel */
							/* boot start */
							/* function */
							/* 1 quad-word */

							/* map initrd using */
							/* 4K pages (4Mb in */
							/* the future) */
#define	E2K_INITRD_PAGE_SIZE		E2K_SMALL_PAGE_SIZE

							/* map bootinfo data */
							/* using 4K pages    */
#define	E2K_BOOTINFO_PAGE_SIZE		E2K_SMALL_PAGE_SIZE

							/* map MP tables */
							/* using 4K pages */
#define	E2K_MPT_PAGE_SIZE		E2K_SMALL_PAGE_SIZE

							/* map symbols & */
							/* strings tables */
							/* using 4K pages    */
#define	E2K_NAMETAB_PAGE_SIZE		E2K_SMALL_PAGE_SIZE

							/* map x86 HW area   */
							/* using 4K pages    */
#define	E2K_X86_HW_PAGE_SIZE		E2K_SMALL_PAGE_SIZE

/*
 * All or some parts of physical memory pages are mapped to virtual
 * space starting from 'PAGE_OFFSET'
 */
#define	E2K_MAPPED_PHYS_MEM_SIZE	(0 * (1024 * 1024))
							/* full physical */
							/* memory */

/* Size of pages to map physical memory */
#define	E2K_MAPPED_PHYS_MEM_PAGE_SIZE (cpu_has(CPU_HWBUG_LARGE_PAGES) ? \
			E2K_SMALL_PAGE_SIZE : E2K_LARGE_PAGE_SIZE)
#define	BOOT_E2K_MAPPED_PHYS_MEM_PAGE_SIZE \
		(boot_cpu_has(CPU_HWBUG_LARGE_PAGES) ? \
				E2K_SMALL_PAGE_SIZE : BOOT_E2K_LARGE_PAGE_SIZE)
#define	E2K_CNTP_MAPPED_MEM_PAGE_SIZE	E2K_MAPPED_PHYS_MEM_PAGE_SIZE

/*
 * Size of the top of kernel stack to map to equal virtual addresses to ensure
 * switching from physical to virtual addressing
 */
#ifndef __ASSEMBLY__
#define	E2K_KERNEL_US_PAGE_SWITCHING_SIZE	(128 * sizeof(long))
#else
#define	E2K_KERNEL_US_PAGE_SWITCHING_SIZE	(128 * 8)
#endif /* !(__ASSEMBLY__) */

/*
 * Kernel virtual memory layout
 */

/*
 * The topmost virtual addresses are used to allocate Virtually Mapped 
 * Linear Page Tables (VM LPT).
 * All page tables is virtually mapped into the same virtual space as kernel
 * Definition of Virtually Mapped Linear Page Table base address.
 * Virtual page table lives at the end of virtual addresses space
 * 0x0000 ff80 0000 0000 - 0x0000 ffff ffff ffff all PTs virtual space:
 *
 * 0x0000 ff80 0000 0000 - 0x0000 ffff bfff ffff third-level PTs (PTEs)
 * 0x0000 ffff c000 0000 - 0x0000 ffff ffdf ffff second-level PTs (PMLDs)
 * 0x0000 ffff ffe0 0000 - 0x0000 ffff ffff efff first-level PTs (PMHDs)
 * 0x0000 ffff ffff f000 - 0x0000 ffff ffff fffe root-level PTs (PGD)
 * 0x0000 ffff ffff ffff - root-level itself PGD
 */

#define	KERNEL_VMLPT_BASE_ADDR		0x0000ff8000000000UL
#ifndef __ASSEMBLY__
#define	KERNEL_ROOT_PTB_BASE_ADDR	((e2k_addr_t)boot_root_pt)
#else
#define	KERNEL_ROOT_PTB_BASE_ADDR	(boot_root_pt)
#endif /* !(__ASSEMBLY__) */

/*
 * Area dedicated for I/O ports and BIOS physical memory
 * 0x0000 00ff fe00 0000 - 0x0000 00ff ffff ffff all I/O physical memory
 * 0x0000 ff7b e000 0000 - 0x0000 ff7b ffff ffff all I/O virtual memory
 */

#define	E2K_KERNEL_IO_BIOS_AREAS_BASE	0x0000ff7be0000000UL
#define	E2K_KERNEL_IO_BIOS_AREAS_SIZE	0x0000000020000000UL

/*
 * E2K I/O ports, BIOS areas physical memory layout
 */

#define	E2K_MAIN_MEM_REGION_START	0x0000000000000000UL	/* from 0 */
#define	E2K_MAIN_MEM_REGION_END		0x0000000080000000UL	/* up to 2Gb */
#define	E2K_EXT_MEM_REGION_START	0x0000000100000000UL	/* from 4Gb */
#define	E2K_EXT_MEM_REGION_END		0x0000001000000000UL	/* up to 64Gb */
#define	E2K_X86_LOW_IO_AREA_PHYS_BASE	0x00000000000A0000UL	/* from 640Kb */
#define	E2K_X86_LOW_IO_AREA_SIZE	0x0000000000060000UL	/* to 1Mb */


/*
 * See BUG in pcim configuration block in jump func
 * should be 0x00000000FEBFFFFFUL due to specification
 */
#define E2K_PCI_MEM_AREA_PHYS_END		0x00000000F7FFFFFFUL
#define E2K_SCRB_SIZE				0x0000000000001000UL

#define	E2K_FULL_SIC_BIOS_AREA_PHYS_BASE	0x0000000100000000UL
#define	E2K_FULL_SIC_BIOS_AREA_SIZE		0x0000000001000000UL
#define	E2K_FULL_SIC_CPU_STARTUP_ADDR		E2K_FULL_SIC_BIOS_AREA_PHYS_BASE
#define	E2K_FULL_SIC_IO_AREA_PHYS_BASE		0x0000000101000000UL
#define	E2K_FULL_SIC_IO_AREA_SIZE		0x0000000000010000UL /* 64K */

#define	E2K_LEGACY_SIC_BIOS_AREA_PHYS_BASE	0x000000fff0000000UL
#define	E2K_LEGACY_SIC_BIOS_AREA_SIZE		0x0000000010000000UL /* 256M */
#define	E2K_LEGACY_SIC_CPU_STARTUP_ADDR		\
		E2K_LEGACY_SIC_BIOS_AREA_PHYS_BASE
#define	E2K_LEGACY_SIC_IO_AREA_PHYS_BASE	0x000000ff20000000UL
#define	E2K_LEGACY_SIC_IO_AREA_SIZE		0x0000000010000000UL /* 256M */

#define E3S_NSR_AREA_PHYS_BASE		0x0000000200000000UL	/* node 0 */
#define E3S_NSR_AREA_SIZE		0x0000000020000000UL	/* size of 1 */
								/* node SR */
								/* area */
#define	E3S_PCICFG_AREA_OFFSET		0x0000000000000000UL	/* offset of */
								/* PCI CFG */
								/* area at */
								/* NSR */
#define	E3S_PCICFG_AREA_SIZE		0x0000000010000000UL	/* size of */
								/* PCI CFG */
								/* area */
#define	E3S_SBSR_AREA_OFFSET		0x0000000010000000UL	/* offset of */
								/* SB (IOHUB) */
								/* registers */
								/* area */
#define	E3S_SBSR_AREA_SIZE		0x0000000000010000UL	/* size of */
								/* IOHUB */
								/* registers */
								/* area */
#define	E3S_NBSR_AREA_OFFSET		0x0000000014000000UL	/* offset of */
								/* NB regs */
								/* into NSR */
								/* area */
#define	E3S_NBSR_AREA_SIZE		0x0000000000010000UL	/* size of */
								/* NBSR area */
								/* of 1 node */
#define ES2_NSR_AREA_PHYS_BASE		0x0000000110000000UL	/* node 0 */
#define ES2_NSR_AREA_MAX_SIZE		0x0000000010000000UL	/* max NSRs */
								/* area: */
								/* limited */
								/* by follow */
								/* LAPICINT */
#define	ES2_NBSR_AREA_OFFSET		0x0000000000000000UL	/* offset of */
								/* NB regs */
								/* into NSR */
								/* area */
#define	ES2_NBSR_AREA_SIZE		0x0000000000100000UL	/* size of */
								/* NBSR area */
								/* of 1 node */
#define ES2_COPSR_AREA_PHYS_BASE	0x00000001c0000000UL	/* Co-Proc */
								/* area start */
								/* DSP 0 of */
								/* node 0 */
#define	ES2_COPSR_AREA_SIZE		0x0000000001000000UL	/* Co-Proc */
								/* one node */
								/* area size */
#define	ES2_PCICFG_AREA_PHYS_BASE	0x0000000200000000UL	/* start of */
								/* PCI CFG */
								/* area */
#define	ES2_PCICFG_AREA_SIZE		0x0000000010000000UL	/* size of */
								/* PCI CFG */
								/* area of */
								/* 1 IO link */
#define	E1CP_PCICFG_AREA_PHYS_BASE	0x000000ff10000000UL	/* start of */
								/* PCI CFG */
								/* area */
#define	E1CP_PCICFG_AREA_SIZE		0x0000000010000000UL	/* 256M */
								/* size of */
								/* PCI CFG */
								/* area of */
								/* 1 IO link */
/* on e2s same as on es2 */
#define E2S_NSR_AREA_PHYS_BASE		ES2_NSR_AREA_PHYS_BASE
#define E2S_NSR_AREA_MAX_SIZE		ES2_NSR_AREA_MAX_SIZE
#define	E2S_NBSR_AREA_OFFSET		ES2_NBSR_AREA_OFFSET
#define	E2S_NBSR_AREA_SIZE		ES2_NBSR_AREA_SIZE
#define E2S_COPSR_AREA_PHYS_BASE	ES2_COPSR_AREA_PHYS_BASE
#define	E2S_COPSR_AREA_SIZE		ES2_COPSR_AREA_SIZE
#define	E2S_PCICFG_AREA_PHYS_BASE	ES2_PCICFG_AREA_PHYS_BASE
#define	E2S_PCICFG_AREA_SIZE		ES2_PCICFG_AREA_SIZE
/* on e8c same as on es2 & e2s */
#define E8C_NSR_AREA_PHYS_BASE		ES2_NSR_AREA_PHYS_BASE
#define E8C_NSR_AREA_MAX_SIZE		ES2_NSR_AREA_MAX_SIZE
#define	E8C_NBSR_AREA_OFFSET		ES2_NBSR_AREA_OFFSET
#define	E8C_NBSR_AREA_SIZE		ES2_NBSR_AREA_SIZE
#define E8C_COPSR_AREA_PHYS_BASE	ES2_COPSR_AREA_PHYS_BASE
#define	E8C_COPSR_AREA_SIZE		ES2_COPSR_AREA_SIZE
#define	E8C_PCICFG_AREA_PHYS_BASE	ES2_PCICFG_AREA_PHYS_BASE
#define	E8C_PCICFG_AREA_SIZE		ES2_PCICFG_AREA_SIZE
/* on e8c2 same as on es2 & e2s & e8c */
#define E8C2_NSR_AREA_PHYS_BASE		ES2_NSR_AREA_PHYS_BASE
#define E8C2_NSR_AREA_MAX_SIZE		ES2_NSR_AREA_MAX_SIZE
#define	E8C2_NBSR_AREA_OFFSET		ES2_NBSR_AREA_OFFSET
#define	E8C2_NBSR_AREA_SIZE		ES2_NBSR_AREA_SIZE
#define E8C2_COPSR_AREA_PHYS_BASE	ES2_COPSR_AREA_PHYS_BASE
#define	E8C2_COPSR_AREA_SIZE		ES2_COPSR_AREA_SIZE
#define	E8C2_PCICFG_AREA_PHYS_BASE	ES2_PCICFG_AREA_PHYS_BASE
#define	E8C2_PCICFG_AREA_SIZE		ES2_PCICFG_AREA_SIZE
/* on e1c+ legacy e2s SIC is used */
#define E1CP_NSR_AREA_PHYS_BASE		early_get_legacy_nbsr_base()
#define E1CP_NSR_AREA_MAX_SIZE		E2S_NSR_AREA_MAX_SIZE
#define	E1CP_NBSR_AREA_OFFSET		E2S_NBSR_AREA_OFFSET
#define	E1CP_NBSR_AREA_SIZE		E2S_NBSR_AREA_SIZE

#define	E3M_X86_IO_AREA_PHYS_BASE	0x000000fff0000000UL
#define	E3M_IO_AREA_PHYS_BASE		0x000000fff0010000UL
#define	E3M_IO_AREA_SIZE		(64 * (1024 * 1024))	/* 64 MB */
#define	E3M_BIOS_AREA_PHYS_BASE		0x000000ffff000000UL
#define	E3M_BIOS_AREA_SIZE		0x0000000001000000UL
#define	E3M_CPU_STARTUP_ADDR		0x000000fffffff800UL

/*
 * Area dedicated for I/O ports and BIOS physical memory
 * 0x0000 00ff fe00 0000 - 0x0000 00ff ffff ffff all I/O physical memory
 * 0x0000 ff7b e000 0000 - 0x0000 ff7b ffff ffff all I/O virtual memory
 *
 * see area dedication above :
 *
 * #define	E2K_KERNEL_IO_BIOS_AREAS_BASE	0x0000ff7be0000000UL
 * #define	E2K_KERNEL_IO_BIOS_AREAS_SIZE	0x0000000020000000UL
 */

/*
 * Area dedicated for kernel resident image virtual space and virtual space
 * to allocate and load kernel modules.
 * Both this areas should be within 2 ** 30 bits of virtual adresses to provide
 * call of extern functions based on literal displacement DISP
 * 0x0000 e200 0000 0000 - 0x0000 e200 3fff ffff kernel image area with modules
 * 0x0000 e200 0000 0000 - 0x0000 e200 0xxx x000 kernel image area
 *					xxx x	 defined by kernel_image_size
 * 0x0000 e200 0xxx x000 - 0x0000 e200 3fff ffff area to load modules
 */
#define	E2K_KERNEL_IMAGE_AREA_BASE	0x0000e20000000000
#define	E2K_KERNEL_IMAGE_AREA_SIZE	kernel_image_size
#define	E2K_MODULE_START		_PAGE_ALIGN_DOWN( \
						(E2K_KERNEL_IMAGE_AREA_BASE + \
						E2K_KERNEL_IMAGE_AREA_SIZE), \
						E2K_KERNEL_PAGE_SIZE)
#define	E2K_MODULE_END			(E2K_KERNEL_IMAGE_AREA_BASE + (1 << 30))
#define	E2K_KERNEL_AREAS_SIZE		0x0000000040000000	/* 2 ** 30 */
#ifdef	CONFIG_KERNEL_CODE_CONTEXT
#define	KERNEL_CODES_INDEX		1UL	/* kernel protected functions */
						/* CUIR value */
#define	MAX_KERNEL_CODES_UNITS		(KERNEL_CODES_INDEX + 1)
#define	USER_CODES_START_INDEX		(0UL)
#define	USER_CODES_32_INDEX		(KERNEL_CODES_INDEX + 1)
#define	USER_CODES_PROT_INDEX		(KERNEL_CODES_INDEX + 1)
#else	/* ! CONFIG_KERNEL_CODE_CONTEXT */
#define	KERNEL_CODES_INDEX		0UL	/* kernel CUIR value */
#define	USER_CODES_START_INDEX		0UL	/* user CUIR value */
#define	USER_CODES_32_INDEX		1UL	/* user 32-bits codes index */
#define	USER_CODES_PROT_INDEX		1UL	/* user protected codes */
						/* index */
#endif	/* CONFIG_KERNEL_CODE_CONTEXT */


/*
 * Area dedicated for kernel symbols & strings tables
 * 0x0000 e200 4000 0000 - 0x0000 e200 ffff ffff kernel symbols & strings tables
 */
#define	E2K_KERNEL_NAMETAB_AREA_BASE	(E2K_KERNEL_IMAGE_AREA_BASE + \
						E2K_KERNEL_AREAS_SIZE)
#define	E2K_KERNEL_NAMETAB_AREAS_SIZE	0x00000000c0000000UL

/*
 * Area dedicated for physical memory mapping to virtual space
 * 0x0000 0000 0000 0000 - 0x0000 00ff ffff ffff all physical memory
 * 0x0000 1000 0000 0000 - 0x0000 10ff ffff ffff all virtual memory to map
 *						 all physical memory
 */
#define	E2K_KERNEL_PHYS_MEM_VIRT_BASE	PAGE_OFFSET /*0x0000200000000000 */
#define	E2K_KERNEL_PHYS_MEM_SIZE	0x0000010000000000UL /* == 2 ** 40 */

#ifdef CONFIG_DEBUG_PAGEALLOC
/*
 * Reserve some memory from the begining of physical memory
 * This memory was mapped to small pages (from physical
 * memory start to start of X86 low IO memory area)
 * Freed memory will be used to split first large pages
 */
#define	DEBUG_PAGEALLOC_AREA_SIZE	(128 * PAGE_SIZE)
#endif	/* CONFIG_DEBUG_PAGEALLOC */

/*
 * Kernel virtual memory context
 */
#define	E2K_KERNEL_CONTEXT		0x000

/*
 * CPU 'WAIT' operation fields structure
 */
#define	E2K_WAIT_OP_MA_C_MASK	0x20	/* wait for all previous memory */
					/* access operatons complete */
#define	E2K_WAIT_OP_FL_C_MASK	0x10	/* wait for all previous flush */
					/* cache operatons complete */
#define	E2K_WAIT_OP_LD_C_MASK	0x08	/* wait for all previous load */
					/* operatons complete */
#define	E2K_WAIT_OP_ST_C_MASK	0x04	/* wait for all previous store */
					/* operatons complete */
#define	E2K_WAIT_OP_ALL_E_MASK	0x02	/* wait for all previous operatons */
					/* issue all possible exceptions */
#define	E2K_WAIT_OP_ALL_C_MASK	0x01	/* wait for all previous operatons */
					/* complete */
#define	E2K_WAIT_OP_ALL_MASK	(E2K_WAIT_OP_MA_C_MASK |	\
				E2K_WAIT_OP_FL_C_MASK |		\
				E2K_WAIT_OP_LD_C_MASK |		\
				E2K_WAIT_OP_ST_C_MASK |		\
				E2K_WAIT_OP_ALL_C_MASK |	\
				E2K_WAIT_OP_ALL_E_MASK)

#define	E2K_WAIT_MA		E2K_WAIT(E2K_WAIT_OP_MA_C_MASK)
#define	E2K_WAIT_FLUSH		E2K_WAIT(E2K_WAIT_OP_FL_C_MASK)
#define	E2K_WAIT_LD		E2K_WAIT(E2K_WAIT_OP_LD_C_MASK)
#define	E2K_WAIT_ST		E2K_WAIT(E2K_WAIT_OP_ST_C_MASK)
#define	E2K_WAIT_ALL_OP		E2K_WAIT(E2K_WAIT_OP_ALL_C_MASK)
#define	E2K_WAIT_ALL_EX		E2K_WAIT(E2K_WAIT_OP_ALL_E_MASK)
#define	E2K_WAIT_ALL		E2K_WAIT(E2K_WAIT_OP_ALL_MASK)

#endif /* !(_E2K_HEAD_H) */
