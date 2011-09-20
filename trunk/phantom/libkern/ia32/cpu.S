#include <ia32/asm.h>

// Load segment limit
ENTRY(ia32_lsl)
	movl	4(%esp),%eax		/* get selector */
	lsl	%eax,%eax
	ret


/*
 * Set cr3
 */
ENTRY(set_cr3)
	movl	4(%esp),%eax		/* get new cr3 value */
	movl	%eax,%cr3		/* load it */
	ret

/*
 * Read cr3
 */
ENTRY(get_cr3)
	movl	%cr3,%eax
	ret

/*
 * Flush TLB
 */
ENTRY(flush_tlb)
	movl	%cr3,%eax		/* flush tlb by reloading CR3 */
	movl	%eax,%cr3		/* with itself */
	ret

/*
 * Read cr2
 */
ENTRY(get_cr2)
	movl	%cr2,%eax
	ret

/*
 * Read ldtr
 */
ENTRY(get_ldt)
	xorl	%eax,%eax
	sldt	%ax
	ret

/*
 * Set ldtr
 */
ENTRY(set_ldt)
	lldt	4(%esp)
	ret

/*
 * Read task register.
 */
ENTRY(get_tr)
	xorl	%eax,%eax
	str	%ax
	ret

#if 0
/*
 * Set task register.  Also clears busy bit of task descriptor.
 */
ENTRY(set_tr)
	movl	S_ARG0,%eax		/* get task segment number */
	subl	$8,%esp			/* push space for SGDT */
	sgdt	2(%esp)			/* store GDT limit and base (linear) */
	movl	4(%esp),%edx		/* address GDT */
	movb	$(K_TSS),5(%edx,%eax)	/* fix access byte in task descriptor */
	ltr	%ax			/* load task register */
	addl	$8,%esp			/* clear stack */
	ret				/* and return */


/*
 * Set task-switched flag.
 */
ENTRY(_setts)
	movl	%cr0,%eax		/* get cr0 */
	orl	$(CR0_TS),%eax		/* or in TS bit */
	movl	%eax,%cr0		/* set cr0 */
	ret
#endif


/* void i386_cpuid(unsigned int selector, unsigned int *data); */
ENTRY(i386_cpuid)
 	pushl	%ebx
 	pushl	%edi
 	movl	12(%esp),%eax
 	movl	16(%esp),%edi
 	cpuid
 	movl	%eax,0(%edi)
 	movl	%ebx,4(%edi)
 	movl	%ecx,8(%edi)
 	movl	%edx,12(%edi)
 	popl	%edi
 	popl	%ebx
 	ret
