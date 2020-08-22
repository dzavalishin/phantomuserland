#ifdef ARCH_ia32

#include <ia32/asm.h>
#if 1
// void rgba2rgba_zbmove( struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos )

//.globl _rgba2rgba_zbmove
//	.def	_rgba2rgba_zbmove;	.scl	2;	.type	32;	.endef
//_rgba2rgba_zbmove:
ENTRY(rgba2rgba_zbmove)
        pushl	%ebp
        movl	%esp, %ebp

        // dest
	movl	8(%ebp), %edi
        // src
        movl	12(%ebp), %esi
        // zb
	movl	16(%ebp), %edx
        // nelem
	movl	20(%ebp), %ecx
        // zpos
	movl	24(%ebp), %ebx

        incl    %ecx // loop is pre-decrement
        jmp     more_pixels

again:
    // *zb
    movl	0(%edx), %eax
    // (*zb <= zpos): eax-ebx: *zb - zbpos
    cmpl	%ebx, %eax
    ja      step_forw

    // *src
    movl	0(%esi), %eax

    roll    $8, %eax
    testb	%al, %al
    rorl    $8, %eax
    je      step_forw

	//movl	0(%esi), %eax
	movl	%eax, 0(%edi)
	movl	%ebx, 0(%edx)

step_forw:
	addl	$4, %edi
	addl	$4, %esi
	addl	$4, %edx

more_pixels:
        loop again

	popl	%ebp
	ret


#endif
#endif
