/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * IA32 startup routine.
 *
**/

	.text

#include "mach/asm.h"

	.globl	_start_of_kernel
_start_of_kernel:
	// dz -- someone needs this from C code as _start... 
	.globl	__start_of_kernel 
__start_of_kernel:
ENTRY(_start)

	jmp	boot_code

	// MultiBoot header - see multiboot.h.  
	P2ALIGN(2)
bootHeader:
	.long	0x1BADB002		/* magic */
#if 1
	.long	0x00000000		/* feature flags */
	//.long	0x00000004		/* feature flags: need VIDEO INFO*/
	.long	0-0x1BADB002-0x00000000	/* checksum */
#else /* a.out kludge */
	.long	0x00010000		/* feature flags */
	.long	0-0x1BADB002-0x00010000	/* checksum */
	.long	bootHeader		/* header address */
	.long	EXT(_start)		/* load address */
	.long	EXT(_bss_start__)	/* address of bss start (load end) */
	.long	EXT(_bss_end__)		/* address of bss end */
	.long	boot_code		/* entry point */
#endif



boot_code:

        // Now set our boot stack
        movl	$EXT(phantom_start_stack_end),%esp

    	// Stack dump (backtrace) code stops at zero ebp
	xorl    %ebp,%ebp

    	// reset cpu flags, most notably d flag
    	pushl	$0
    	popf

	/* clear BSS */
	lea	EXT(_bss_start__),%edi
	lea	EXT(_bss_end__),%ecx
	subl	%edi,%ecx
	xorl	%eax,%eax
	rep
	stosb

	/* Push the boot_info parameter for phantom_multiboot_main()  */
	pushl	%ebx

	/* Jump into C code.  */
	call	EXT(phantom_multiboot_main)
        //call	EXT(multiboot_main)

//#include <x86/phantom_page.h>	
#include <kernel/page.h>
	/* Data seg. Need PAGE_SHIFT defined! */
	.data
	P2ALIGN(PAGE_SHIFT)
	.globl	EXT(start_of_data)
LEXT(start_of_data)
	.long	0


