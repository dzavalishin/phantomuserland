/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * IA32 startup routine.
 *
**/

#include <multiboot2.h>
#include <ia32/asm.h>

    .text


    .globl	_start_of_kernel
_start_of_kernel:
	// dz -- someone needs this from C code as _start...
    .globl	__start_of_kernel
__start_of_kernel:
ENTRY(_start)

    jmp	boot_code

#if 1||HAVE_MULTIBOOT2
    // MultiBoot2 header - see multiboot2.h.
    // We put it before so that mb2 loader will find and use it,
    // and older loader will search for the next one
    P2ALIGN(2)

multiboot2_header:
    /* magic */
    .long MULTIBOOT2_HEADER_MAGIC
    /* ISA: i386 */
    .long MULTIBOOT_ARCHITECTURE_I386
    /* Header length. */
    .long multiboot2_header_end - multiboot2_header
    /* checksum */
    .long -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + (multiboot2_header_end - multiboot2_header))
framebuffer_tag_start:
    .short MULTIBOOT_HEADER_TAG_FRAMEBUFFER
    .short MULTIBOOT_HEADER_TAG_OPTIONAL
    .long framebuffer_tag_end - framebuffer_tag_start
    .long 1280
    .long 1024
    .long 32
framebuffer_tag_end:
    .short MULTIBOOT_HEADER_TAG_END
    .short 0
    .long 8
multiboot2_header_end:
#endif // HAVE_MULTIBOOT2

// feature flags: need VIDEO INFO
#define MB1_FLAGS 0x00000004

// feature flags: none
//#define MB1_FLAGS 0x00000000

    // MultiBoot header - see multiboot.h.
    P2ALIGN(2)
bootHeader:
    .long	0x1BADB002		/* magic */
#if 1
    .long	MB1_FLAGS		
    .long	0-0x1BADB002-MB1_FLAGS	/* checksum */

    .long	bootHeader		/* header address */
    .long	EXT(_start)		/* load address */
    .long	EXT(_bss_start__)	/* address of bss start (load end) */
    .long	EXT(_bss_end__)		/* address of bss end */
    .long	boot_code		/* entry point */

    // Videomode
    .long       0
    //x*y
    .long       1280
    .long       1024
    // bpp
    .long       32

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
    // Save eax - mb2 sends magic cookie there
    movl        %eax, %esi
    // Now set our boot stack
    movl	$EXT(phantom_start_stack_end),%esp

    // Stack dump (backtrace) code stops at zero ebp
    xorl        %ebp,%ebp

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

    // Push the magic cookie we got in eax
    pushl	%esi

    // Push the boot_info parameter for phantom_multiboot_main()
    pushl	%ebx

    /* Jump into C code.  */
    call	EXT(phantom_multiboot_main)
    //call	EXT(multiboot_main)

#include <kernel/page.h>
    /* Data seg. Need PAGE_SHIFT defined! */
    .data
    P2ALIGN(PAGE_SHIFT)
    .globl	EXT(start_of_data)
    LEXT(start_of_data)
    .long	0






#if 0




    // -----------------------------------------------------------------------
    // Call 16 bit PM code. Used in PM vesa interface.
    // -----------------------------------------------------------------------
    //
    // errno_t call_16bit_code( u_int32_t cs, u_int32_t ss, u_int32_t entry, struct trap_state *ts );
    //
    // NB! Passed CS must be 16 bit selector. That's all we need to switch to 16 bit mode
    //
    // NB! Must be called with ints disabled!
    //
    // -----------------------------------------------------------------------

    .data
    // We store here 32 bit ESP when calling PM16
saved_pm16_esp:
    .long	0

saved_pm16_entry:
    .long	0

saved_pm16_ss:
    .long	0

saved_pm16_cs:
    .long	0

    .text
    .code32

ENTRY(call_16bit_vesa)
    // Make sure!
    cli

    push %ebp
    movl %esp, %ebp

    pusha


    movl	4(%esp),%eax			// cs
    movl	8(%esp),%ebx			// ss
    movl	12(%esp),%ecx			// entry
    movl	16(%esp),%edx			// *ts

    movl        %eax, saved_pm16_cs(0)
    movl        %ebx, saved_pm16_ss(0)
    movl        %ecx, saved_pm16_entry(0)

    movl        %esp, saved_pm16_esp(0)


    push        %cs
    pushl        $done_vesa_pm16

    //call        *%ecx

    /**
     *
     *    SS
     *    ESP
     *    EFLAGS
     *    CS
     *    EIP
     *
    **/


    iret

    .code16
done_vesa_pm16:

    // restore world

    ljmp KERNEL_CS, $1f
    .code32
1:

    movw KERNEL_DS, %ds
    movw KERNEL_DS, %es
    movw KERNEL_DS, %ss

    movw 0, %fs
    movw 0, %gs

    movl        saved_pm16_esp(0), %esp

    popa

    pop %ebp

    ret

#endif


