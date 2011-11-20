
#ifndef	_AMD64_TRAP_H_
#define	_AMD64_TRAP_H_

#ifndef ARCH_amd64
#warning Intel64 code! Wrong arch?
#endif

#ifndef GENERAL_TRAP_H
#warning include <kernel/trap.h> instead!
#endif

#include <phantom_types.h>

/*
 * Hardware trap vectors for i386.
 * /
#define	T_DIVIDE_ERROR		0
#define	T_DEBUG			1
#define	T_NMI			2		// non-maskable interrupt
#define	T_INT3			3		// int 3 instruction
#define	T_OVERFLOW		4		// overflow test
#define	T_OUT_OF_BOUNDS		5		// bounds check
#define	T_INVALID_OPCODE	6		// invalid op code
#define	T_NO_FPU		7		// no floating point
#define	T_DOUBLE_FAULT		8		// double fault
#define	T_FPU_FAULT		9
#define T_INVALID_TSS		10
#define	T_SEGMENT_NOT_PRESENT	11
#define	T_STACK_FAULT		12
#define	T_GENERAL_PROTECTION	13
#define	T_PAGE_FAULT		14
// Pentium Pro generates this due to a bug.  See the eratta sheet.	15
#define	T_FLOATING_POINT_ERROR	16
#define	T_WATCHPOINT		17
#define	T_ALIGNMENT_CHECK	17
#define	T_MACHINE_CHECK		18

/ *
 * Page-fault trap codes.
 * /
#define	T_PF_PROT		0x1		// protection violation
#define	T_PF_USER		0x4		// from user state
*/

#warning check write bit
#define	T_PF_WRITE		0x2		// write access


#define	T_PRIVINFLT	1	/* privileged instruction */
#define	T_BPTFLT	3	/* breakpoint instruction */
#define	T_ARITHTRAP	6	/* arithmetic trap */
#define	T_PROTFLT	9	/* protection fault */
#define	T_TRCTRAP	10	/* debug exception (sic) */
#define	T_PAGEFLT	12	/* page fault */
#define	T_ALIGNFLT	14	/* alignment fault */

#define	T_DIVIDE	18	/* integer divide fault */
#define	T_NMI		19	/* non-maskable trap */
#define	T_OFLOW		20	/* overflow trap */
#define	T_BOUND		21	/* bound instruction fault */
#define	T_DNA		22	/* device not available fault */
#define	T_DOUBLEFLT	23	/* double fault */
#define	T_FPOPFLT	24	/* fp coprocessor operand fetch fault */
#define	T_TSSFLT	25	/* invalid tss fault */
#define	T_SEGNPFLT	26	/* segment not present fault */
#define	T_STKFLT	27	/* stack fault */
#define	T_MCHK		28	/* machine check trap */
#define	T_XMMFLT	29	/* SIMD floating-point exception */
#define	T_RESERVED	30	/* reserved (unknown) */






#ifndef ASSEMBLER


/* This structure corresponds to the state of user registers
   as saved upon kernel trap/interrupt entry.
 */
/*
struct trap_state {

	// Saved segment registers
	unsigned int	gs;
	unsigned int	fs;
	unsigned int	es;
	unsigned int	ds;

	// PUSHA register state frame
	unsigned int	edi;
	unsigned int	esi;
	unsigned int	ebp;
	unsigned int	cr2;	// we save cr2 over esp for page faults
	unsigned int	ebx;
	unsigned int	edx;
	unsigned int	ecx;
	unsigned int	eax;

	unsigned int	trapno;
	unsigned int	err;

	// Processor state frame
	unsigned int	eip;
	unsigned int	cs;
	unsigned int	eflags;
	unsigned int	esp;
	unsigned int	ss;

	// Virtual 8086 segment registers
	unsigned int	v86_es;
	unsigned int	v86_ds;
	unsigned int	v86_fs;
	unsigned int	v86_gs;
};
*/

struct trap_state {
	register_t	rdi;
	register_t	rsi;
	register_t	rdx;
	register_t	rcx;
	register_t	r8;
	register_t	r9;
	register_t	rax;
	register_t	rbx;
	register_t	rbp;
	register_t	r10;
	register_t	r11;
	register_t	r12;
	register_t	r13;
	register_t	r14;
        register_t	r15;

        register_t	trapno;

	register_t	addr;
	register_t	flags;
	/* below portion defined in hardware */
	register_t	hw_err;
	register_t	hw_rip;
	register_t	hw_cs;
	register_t	hw_rflags;
	register_t	hw_rsp;
	register_t	hw_ss;

};

#define TS_PROGRAM_COUNTER hw_rip


/* The actual trap_state frame pushed by the processor
   varies in size depending on where the trap came from.  */
#define TR_KSIZE	((int)&((struct trap_state*)0)->esp)
#define TR_USIZE	((int)&((struct trap_state*)0)->v86_es)
#define TR_V86SIZE	sizeof(struct trap_state)

//#define I386_N_TRAPS 32
#define ARCH_N_TRAPS 32


//int (*phantom_trap_handlers[I386_N_TRAPS])(struct trap_state *ts);


#endif	// asm





#endif	// _AMD64_TRAP_H_

