#include <ia32/asm.h>

// Phantom native code syscall entry point
ENTRY(syscall)
    pushl	$0                      // 0 -> err
    pushl       %eax                    // syscall no ->trapno

    pusha				/* save the general registers */
    pushl	%ds			/* and the segment registers */
    pushl	%es
    pushl	%fs
    pushl	%gs
        
    mov		%ss,%ax			/* switch to kernel data segment */
    mov		%ax,%ds			/* (same as kernel stack segment) */
    mov		%ax,%es

    pushl	%esp			/* pass parameter */
    call EXT(syscall_sw)
    addl	$4,%esp			/* pop parameter */

    // Now return
    popl	%gs			/* restore segment registers */
    popl	%fs
    popl	%es
    popl	%ds
    popa				/* restore general registers */

    addl	$8,%esp			// discard trap number and error code

    lret
    
