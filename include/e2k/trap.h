#warning write me


#ifndef	_E2K_TRAP_H_
#define	_E2K_TRAP_H_

#ifndef ARCH_e2k
#warning Wrong arch? Expect e2k
#endif

#ifndef GENERAL_TRAP_H
#warning include <kernel/trap.h> instead!
#endif

#include <phantom_types.h>

/*
 * Trap Info Register: the numbers of exceptions
 */

#define	exc_illegal_opcode_num		0	/* 00 */
#define	exc_priv_action_num		1	/* 01 */
#define	exc_fp_disabled_num		2	/* 02 */
#define	exc_fp_stack_u_num		3	/* 03 */
#define	exc_d_interrupt_num		4	/* 04 */
#define	exc_diag_ct_cond_num		5	/* 05 */
#define	exc_diag_instr_addr_num		6	/* 06 */
#define	exc_illegal_instr_addr_num	7	/* 07 */
#define	exc_instr_debug_num		8	/* 08 */
#define	exc_window_bounds_num		9	/* 09 */
#define	exc_user_stack_bounds_num	10	/* 10 */
#define	exc_proc_stack_bounds_num	11	/* 11 */
#define	exc_chain_stack_bounds_num	12	/* 12 */
#define	exc_fp_stack_o_num		13	/* 13 */
#define	exc_diag_cond_num		14	/* 14 */
#define	exc_diag_operand_num		15	/* 15 */
#define	exc_illegal_operand_num		16	/* 16 */
#define	exc_array_bounds_num		17	/* 17 */
#define	exc_access_rights_num		18	/* 18 */
#define	exc_addr_not_aligned_num	19	/* 19 */
#define	exc_instr_page_miss_num		20	/* 20 */
#define	exc_instr_page_prot_num		21	/* 21 */
#define	exc_ainstr_page_miss_num	22	/* 22 */
#define	exc_ainstr_page_prot_num	23	/* 23 */
#define	exc_last_wish_num		24	/* 24 */
#define	exc_base_not_aligned_num	25	/* 25 */

#define	exc_data_debug_num		28	/* 28 */
#define	exc_data_page_num		29	/* 29 */

#define	exc_recovery_point_num		31	/* 31 */
#define	exc_interrupt_num		32	/* 32 */
#define	exc_nm_interrupt_num		33	/* 33 */
#define	exc_div_num			34	/* 34 */
#define	exc_fp_num			35	/* 35 */
#define	exc_mem_lock_num		36	/* 36 */
#define	exc_mem_lock_as_num		37	/* 37 */
#define	exc_mem_error_out_cpu_num	38	/* 38 */
#define	exc_mem_error_MAU_num		39	/* 39 */
#define	exc_mem_error_L2_num		40	/* 40 */
#define	exc_mem_error_L1_35_num		41	/* 41 */
#define	exc_mem_error_L1_02_num		42	/* 42 */
#define	exc_mem_error_ICACHE_num	43	/* 43 */


#if 0



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


#endif



#ifndef ASSEMBLER


/* This structure corresponds to the state of user registers
   as saved upon kernel trap/interrupt entry.
 */

// FIXME not done
// TODO do we have all fields?
// TODO field positions must be checked

struct trap_state {
#if 0
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
#endif

	unsigned long long psr;
	unsigned long long upsr;

	unsigned long long oscud_lo;
	unsigned long long oscud_hi;
	unsigned long long osgd_lo;
	unsigned long long osgd_hi;
	unsigned long long osem;
	unsigned long long osr0;

	unsigned long long pfpfr;
	unsigned long long fpcr;
	unsigned long long fpsr;

	unsigned long long usbr;
	unsigned long long usd_lo;
	unsigned long long usd_hi;

	unsigned long long psp_lo;
	unsigned long long psp_hi;
	unsigned long long pshtp;

	unsigned long long cr0_lo;
	unsigned long long cr0_hi;
	unsigned long long cr1_lo;
	unsigned long long cr1_hi;

	unsigned long long cwd;

	unsigned long long pcsp_lo;
	unsigned long long pcsp_hi;
	unsigned long long pcshtp;

	unsigned long long cud_lo;
	unsigned long long cud_hi;
	unsigned long long gd_lo;
	unsigned long long gd_hi;

	unsigned long long cs_lo;
	unsigned long long cs_hi;
	unsigned long long ds_lo;
	unsigned long long ds_hi;
	unsigned long long es_lo;
	unsigned long long es_hi;
	unsigned long long fs_lo;
	unsigned long long fs_hi;
	unsigned long long gs_lo;
	unsigned long long gs_hi;
	unsigned long long ss_lo;
	unsigned long long ss_hi;

	unsigned long long aad[32*2]; /* %aad0.lo, %aad0.hi, %aad1.lo ... */
	unsigned long long aaind[16];
	unsigned long long aaincr[8];
	unsigned long long aaldi[64];
	unsigned long long aaldv;
	unsigned long long aalda[64];
	unsigned long long aaldm;
	unsigned long long aasr;
	unsigned long long aafstr;
	unsigned long long aasti[16];

	unsigned long long clkr;
	unsigned long long dibcr;
	unsigned long long ddbcr;
	unsigned long long dibar[4];
	unsigned long long ddbar[4];
	unsigned long long dimcr;
	unsigned long long ddmcr;
	unsigned long long dimar[2];
	unsigned long long ddmar[2];
	unsigned long long dibsr;
	unsigned long long ddbsr;
	unsigned long long dtcr;
	unsigned long long dtarf;
	unsigned long long dtart;

	unsigned long long wd;

	unsigned long long br;
	unsigned long long bgr;

	unsigned long long ip;
	unsigned long long nip;
	unsigned long long ctpr1;
	unsigned long long ctpr2;
	unsigned long long ctpr3;

	unsigned long long eir;

	unsigned long long tr; /* unused */

	unsigned long long cutd;
	unsigned long long cuir;
	unsigned long long tsd; /* unused */

	unsigned long long lsr;
	unsigned long long ilcr;

	long long	sys_rval; 
	long long	sys_num;
	long long	arg1;	
	long long       arg2;
	long long	arg3;
	long long	arg4;
	long long	arg5;
	long long	arg6;

/*
 * Some space for backup/restore of extensions and tags of global registers.
 * now places in the end of structure
 */
	unsigned char	gtag[32];
	unsigned short	gext[32];
/*
 *  additional part (for binary compiler)
 */          
        unsigned long long rpr_hi;
        unsigned long long rpr_lo;
/* TODO define sizes        
        unsigned long long tir_lo [TIR_NUM];
	unsigned long long tir_hi [TIR_NUM];

	unsigned long long trap_cell_addr [MAX_TC_SIZE];
	unsigned long long trap_cell_val  [MAX_TC_SIZE];
	unsigned char      trap_cell_tag  [MAX_TC_SIZE];
	unsigned long long trap_cell_info [MAX_TC_SIZE];

	unsigned long long dam [DAM_ENTRIES_NUM];

	unsigned long long sbbp [SBBP_ENTRIES_NUM];
        
	unsigned long long mlt [MLT_NUM];
*/

/*
// intel regs

	u_int64_t cs_lo;
	u_int64_t cs_hi;
	u_int64_t ds_lo;
	u_int64_t ds_hi;
	u_int64_t es_lo;
	u_int64_t es_hi;
	u_int64_t fs_lo;
	u_int64_t fs_hi;
	u_int64_t gs_lo;
	u_int64_t gs_hi;
	u_int64_t ss_lo;
	u_int64_t ss_hi;
	u_int64_t rpr_lo;
	u_int64_t rpr_hi;
*/

// TODO position!
	int		intno; // interrupt number
	int		trapno; // trapnumber

};

// FIXME right?

#define TS_PROGRAM_COUNTER ip

#if 0

/* The actual trap_state frame pushed by the processor
   varies in size depending on where the trap came from.  */
#define TR_KSIZE	((int)&((struct trap_state*)0)->esp)
#define TR_USIZE	((int)&((struct trap_state*)0)->v86_es)
#define TR_V86SIZE	sizeof(struct trap_state)

#endif

// Linux e2k port says so in traps.h
#define ARCH_N_TRAPS 43




#endif	// asm





#endif	// _E2K_TRAP_H_

