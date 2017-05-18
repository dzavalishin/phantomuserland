#warning write me

#ifndef ARCH_e2k
#warning Elbrus2k code! Wrong arch?
#endif



// -------------------- unsure about what is below



#define TEXT_ALIGN	4
#define DATA_ALIGN	2
#define ALIGN		TEXT_ALIGN

#define P2ALIGN(p2)	.p2align p2	/* gas-specific */

#define	LCL(x)	x

#define LB(x,n) n
#ifdef	__STDC__
#ifndef __ELF__
#define EXT(x) _ ## x
#define LEXT(x) _ ## x ## :
#define SEXT(x) "_"#x
#else
#define EXT(x) x
#define LEXT(x) x ## :
#define SEXT(x) #x
#endif
#define LCLL(x) x ## :
#define gLB(n)  n ## :
#define LBb(x,n) n ## b
#define LBf(x,n) n ## f
#else // __STDC__
#error XXX elf
#define EXT(x) _/**/x
#define LEXT(x) _/**/x/**/:
#define LCLL(x) x/**/:
#define gLB(n) n/**/:
#define LBb(x,n) n/**/b
#define LBf(x,n) n/**/f
#endif // __STDC__

//#define SVC .byte 0x9a; .long 0; .word 0x7

//#define String	.ascii
//#define Value	.word
//#define Times(a,b) (a*b)
//#define Divide(a,b) (a/b)


#define __FBSDID(x)
#define END(x)

// TODO write gprof version? we're not using it really


#ifdef GPROF

#error not implemented for e2k

#define MCOUNT		.data; gLB(9) .long 0; .text; lea LBb(x, 9),%edx; call mcount
#define	ENTRY(x)	.globl EXT(x); .p2align TEXT_ALIGN; LEXT(x) ; \
			pushl %ebp; movl %esp, %ebp; MCOUNT; popl %ebp;
#define	ENTRY2(x,y)	.globl EXT(x); .globl EXT(y); \
			.p2align TEXT_ALIGN; LEXT(x) LEXT(y)
#define	ASENTRY(x) 	.globl x; .p2align TEXT_ALIGN; gLB(x) ; \
  			pushl %ebp; movl %esp, %ebp; MCOUNT; popl %ebp;

#define NON_GPROF_ENTRY(x) .globl EXT(x); .p2align TEXT_ALIGN; LEXT(x)

#else	// GPROF

#define MCOUNT
#define	ENTRY(x)	.globl EXT(x); .p2align TEXT_ALIGN; LEXT(x)
#define	ENTRY2(x,y)	.globl EXT(x); .globl EXT(y); \
			.p2align TEXT_ALIGN; LEXT(x) LEXT(y)
#define	ASENTRY(x)	.globl x; .p2align TEXT_ALIGN; gLB(x)

#define NON_GPROF_ENTRY(x) .globl EXT(x); .p2align TEXT_ALIGN; LEXT(x)


#endif	// GPROF

#define	Entry(x)	.globl EXT(x); .p2align TEXT_ALIGN; LEXT(x)
#define	DATA(x)		.globl EXT(x); .p2align DATA_ALIGN; LEXT(x)


