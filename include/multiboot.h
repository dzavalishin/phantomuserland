/* 
 * Copyright (c) 1995-1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 *      Author: Bryan Ford, University of Utah CSL
 */
#ifndef _MACH_I386_MULTIBOOT_H_
#define _MACH_I386_MULTIBOOT_H_

#include <phantom_types.h>





/* The entire multiboot_header must be contained
   within the first MULTIBOOT_SEARCH bytes of the kernel image.  */
#define MULTIBOOT_SEARCH	8192

/* Magic value identifying the multiboot_header.  */
#define MULTIBOOT_MAGIC		0x1badb002


/* Features flags for 'flags'.
   If a boot loader sees a flag in MULTIBOOT_MUSTKNOW set
   and it doesn't understand it, it must fail.  */
#define MULTIBOOT_MUSTKNOW	0x0000ffff

/* Align all boot modules on page (4KB) boundaries.  */
#define MULTIBOOT_PAGE_ALIGN	0x00000001

/* Must be provided memory information in multiboot_info structure */
#define MULTIBOOT_MEMORY_INFO	0x00000002



/* Has video mode information   */
#define MULTIBOOT_VIDEO_MODE	0x00000004



/* Use the load address fields above instead of the ones in the a.out header
   to figure out what to load where, and what to do afterwards.
   This should only be needed for a.out kernel images
   (ELF and other formats can generally provide the needed information).  */
#define MULTIBOOT_AOUT_KLUDGE	0x00010000

/* The boot loader passes this value in register EAX to signal the kernel
   that the multiboot method is being used */
#define MULTIBOOT_VALID         0x2badb002














#define MULTIBOOT_MEMORY	0x00000001
#define MULTIBOOT_BOOT_DEVICE	0x00000002
#define MULTIBOOT_CMDLINE	0x00000004
#define MULTIBOOT_MODS		0x00000008
#define MULTIBOOT_AOUT_SYMS	0x00000010
#define MULTIBOOT_ELF_SHDR	0x00000020
#define MULTIBOOT_MEM_MAP	0x00000040

// dz newer ones:

#define MULTIBOOT_DRIVE_INFO	(1L<<7)
#define MULTIBOOT_CONFIG_TABLE	(1L<<8)
#define MULTIBOOT_BOOT_LOADER_NAME	(1L<<9)
#define MULTIBOOT_APM_TABLE	(1L<<10)
#define MULTIBOOT_VIDEO_INFO	(1L<<11)




#ifndef ASSEMBLER

/* For a.out kernel boot images, the following header must appear
   somewhere in the first 8192 bytes of the kernel image file.  */
struct multiboot_header
{
	/* Must be MULTIBOOT_MAGIC */
	unsigned		magic;

	/* Feature flags - see below.  */
	unsigned		flags;

	/*
	 * Checksum
	 *
	 * The above fields plus this one must equal 0 mod 2^32.
	 */
	unsigned		checksum;

	/* These are only valid if MULTIBOOT_AOUT_KLUDGE is set.  */
	physaddr_t		header_addr;
	physaddr_t		load_addr;
	physaddr_t		load_end_addr;
	physaddr_t		bss_end_addr;
	physaddr_t		entry;

        /* dz: added new -- These are only valid if MULTIBOOT_VIDEO_MODE is set.  */
        u_int32_t 		mode_type;
        u_int32_t 		width;
        u_int32_t 		height;
        u_int32_t 		depth;

};






/* The boot loader passes this data structure to the kernel in
   register EBX on entry.  */
struct multiboot_info
{
	/* These flags indicate which parts of the multiboot_info are valid;
	   see below for the actual flag bit definitions.  */
	unsigned		flags;

	/* Lower/Upper memory installed in the machine.
	   Valid only if MULTIBOOT_MEMORY is set in flags word above.  */
	u_int32_t		mem_lower;
	u_int32_t		mem_upper;

	/* BIOS disk device the kernel was loaded from.
	   Valid only if MULTIBOOT_BOOT_DEVICE is set in flags word above.  */
	unsigned char		boot_device[4];

	/* Command-line for the OS kernel: a null-terminated ASCII string.
	   Valid only if MULTIBOOT_CMDLINE is set in flags word above.  */
	physaddr_t		cmdline;

	/* List of boot modules loaded with the kernel.
	   Valid only if MULTIBOOT_MODS is set in flags word above.  */
	unsigned		mods_count;
	physaddr_t		mods_addr;

	/* Symbol information for a.out or ELF executables. */
	union
	{
	  struct
	  {
	    /* a.out symbol information valid only if MULTIBOOT_AOUT_SYMS
	       is set in flags word above.  */
	    u_int32_t		tabsize;
	    u_int32_t		strsize;
	    physaddr_t		addr;
	    unsigned		reserved;
	  } a;

	  struct
	  {
	    /* ELF section header information valid only if
	       MULTIBOOT_ELF_SHDR is set in flags word above.  */
	    unsigned		num;
	    u_int32_t		size;
	    physaddr_t		addr;
	    unsigned		shndx;
	  } e;
	} syms;

	/* Memory map buffer.
	   Valid only if MULTIBOOT_MEM_MAP is set in flags word above.  */
	u_int32_t		mmap_count;
	physaddr_t		mmap_addr;

        // dz: new multibut stuff follows, be careful

        /* Drive Info buffer */
        u_int32_t 		drives_length;
        u_int32_t 		drives_addr;

        /* ROM configuration table */
        u_int32_t 		config_table;

        /* Boot Loader Name */
        u_int32_t 		boot_loader_name;

        /* APM table */
        u_int32_t 		apm_table;

        /* Video */
        u_int32_t 		vbe_control_info;
        u_int32_t 		vbe_mode_info;
        u_int32_t 		vbe_mode;
        u_int32_t 		vbe_interface_seg;
        u_int32_t 		vbe_interface_off;
        u_int32_t 		vbe_interface_len;

};













/* The mods_addr field above contains the physical address of the first
   of 'mods_count' multiboot_module structures.  */
struct multiboot_module
{
	/* Physical start and end addresses of the module data itself.  */
	physaddr_t		mod_start;
	physaddr_t		mod_end;

	/* Arbitrary ASCII string associated with the module.  */
	physaddr_t		string;

	/* Boot loader must set to 0; OS must ignore.  */
	unsigned		reserved;
};


/* The mmap_addr field above contains the physical address of the first
   of the AddrRangeDesc structure.  "size" represents the size of the
   rest of the structure and optional padding.  The offset to the beginning
   of the next structure is therefore "size + 4".  */
struct AddrRangeDesc
{
  unsigned long size;
  unsigned long BaseAddrLow;
  unsigned long BaseAddrHigh;
  unsigned long LengthLow;
  unsigned long LengthHigh;
  unsigned long Type;

  /* unspecified optional padding... */
};

#endif // ASSEMBLER

/* usable memory "Type", all others are reserved.  */
#define MB_ARD_MEMORY       1



//extern struct multiboot_info bootParameters;
struct multiboot_module *phantom_multiboot_find(const char *string);


#endif //_MACH_I386_MULTIBOOT_H_
