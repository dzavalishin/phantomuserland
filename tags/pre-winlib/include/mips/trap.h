
#ifndef	_ARM_TRAP_H_
#define	_ARM_TRAP_H_

#ifndef ARCH_mips
#warning MIPS code! Wrong arch?
#endif

#ifndef GENERAL_TRAP_H
#warning include <kernel/trap.h> instead!
#endif

#define ARCH_N_TRAPS            37


//#define	T_RESET                 (16+0) // can't happen
#define	T_CACHE_ERR             (32+1)
#define	T_TLB                   (32+2)
#define	T_XTLB                  (32+3)
//#define	T_OTHER                 4

#define T_INTERRUPT             0 // HW interrupt

#define	T_TLB_MOD               1 // TLB modification
#define	T_TLB_LOAD              2 // TLB load
#define	T_TLB_STORE             3 // TLB store

#define T_ADDR_LOAD             4 // Address error on data load or inst fetch
#define T_ADDR_SAVE             5 // Address error on data store
#define T_CODE_BUS_ERR          6 // Bus error accessing instruction
#define T_DATA_BUS_ERR          7 // Bus error accessing data
#define T_SYSCALL               8 // Syscall :)
#define T_BREAK                 9 // Breakpoint
#define T_NO_INSTR             10 // Reserved opcode
#define T_NO_CP                11 // No coprocessor
#define T_OVERFLOW             12 // Arithmetic overflow
#define T_TRAP                 13 // ?

#define T_FPE                  15 // Floating point exception


#define TRAP_STATE_SIZE (40*4)

#ifndef ASSEMBLER


/*
 * This structure corresponds to the state of user registers
 * as saved upon kernel trap/interrupt entry.
*/


// Push from bottom of struct (upper addresses) to top

struct trap_state
{
    unsigned int r0;   // zero, but keep to have aligned count
    unsigned int r1;   // AT - asm temp
    unsigned int r2;   // v0 - retval
    unsigned int r3;   // v1 - ret hi
    unsigned int r4;   // a0 - arg 0
    unsigned int r5;   // a1 - arg 1
    unsigned int r6;   // a2 - arg 2
    unsigned int r7;   // a3 - arg 3
    unsigned int r8;   // t0 - func temp, saved by caller
    unsigned int r9;
    unsigned int r10;
    unsigned int r11;
    unsigned int r12;
    unsigned int r13;
    unsigned int r14;
    unsigned int r15;
    unsigned int r16;  // t0 - func var, saved by callee
    unsigned int r17;
    unsigned int r18;
    unsigned int r19;
    unsigned int r20;
    unsigned int r21;
    unsigned int r22;
    unsigned int r23;
    unsigned int r24;  // t8
    unsigned int r25;  // t9
    unsigned int r26;  // k0, used only on kernel trap entry
    unsigned int r27;  // k1, used only on kernel trap entry
    unsigned int r28;  // gp
    unsigned int r29;  // sp
    unsigned int r30;  // fp/s8
    unsigned int r31;  // ra

    // Offending virtual address for fault kind of exception
    unsigned int va;       // CP0_BADVADDR

    // Exception status
    unsigned int status;   // CP0_STATUS

    // This is not ra, but value stored by exception
    // in special user pc register (epc)
    unsigned int pc;       // CP0_EPC

    unsigned int error_pc; // CP0_ERROREPC

    unsigned int hi;
    unsigned int lo;

    // All of the above are saved in asm. Those below - in C

    unsigned int trapno;
    unsigned int intno;     // 

};


#define TS_PROGRAM_COUNTER pc


//! Called from exception handler on TLB refill events
void tlb_refill_exception(struct trap_state *ts);






#else // ASSEMBLER

//#include <mips/asm.h>


#endif // ASSEMBLER



#endif	// _ARM_TRAP_H_

