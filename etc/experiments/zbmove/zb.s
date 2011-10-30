	.file	"zb.c"
	.comm	___packed, 4, 0
	.text
.globl _rgba2rgba_zbmove
	.def	_rgba2rgba_zbmove;	.scl	2;	.type	32;	.endef
_rgba2rgba_zbmove:
	pushl	%ebp
	movl	%esp, %ebp
	jmp	L2
L4:
	movl	12(%ebp), %eax
	movzbl	3(%eax), %eax
	testb	%al, %al
	je	L3
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	cmpl	24(%ebp), %eax
	ja	L3
	movl	16(%ebp), %eax
	movl	24(%ebp), %edx
	movl	%edx, (%eax)
	movl	8(%ebp), %eax
	movl	12(%ebp), %edx
	movl	(%edx), %edx
	movl	%edx, (%eax)
L3:
	addl	$4, 8(%ebp)
	addl	$4, 12(%ebp)
	addl	$4, 16(%ebp)
L2:
	cmpl	$0, 20(%ebp)
	setg	%al
	subl	$1, 20(%ebp)
	testb	%al, %al
	jne	L4
	popl	%ebp
	ret
