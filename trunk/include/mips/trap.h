
#ifndef	_ARM_TRAP_H_
#define	_ARM_TRAP_H_

#ifndef ARCH_mips
#warning MIPS code! Wrong arch?
#endif

#ifndef GENERAL_TRAP_H
#warning include <kernel/trap.h> instead!
#endif

#define ARCH_N_TRAPS            5

#define	T_RESET                 0
#define	T_CACHE_ERR             1
#define	T_TLB                   2
#define	T_XTLB                  3
#define	T_OTHER                 4




#ifndef ASSEMBLER


/*
 * This structure corresponds to the state of user registers
 * as saved upon kernel trap/interrupt entry.
*/


// Push from bottom of struct (upper addresses) to top

struct trap_state
{
    //unsigned int r0; // zero
    unsigned int r1;
    unsigned int r2;
    unsigned int r3;
    unsigned int r4;
    unsigned int r5;
    unsigned int r6;
    unsigned int r7;
    unsigned int r8;
    unsigned int r9;
    unsigned int r10;
    unsigned int r11;
    unsigned int r12;
    unsigned int r13;
    unsigned int r14;
    unsigned int r15;
    unsigned int r16;
    unsigned int r17;
    unsigned int r18;
    unsigned int r19;
    unsigned int r20;
    unsigned int r21;
    unsigned int r22;
    unsigned int r23;
    unsigned int r24;
    unsigned int r25;
    unsigned int r26;
    unsigned int r27;
    unsigned int r28;
    unsigned int r29;
    unsigned int r30;
    unsigned int r31;
    unsigned int r32;


    unsigned int trapno;
    unsigned int intno;     // 

};










#else // ASSEMBLER

//#include <mips/asm.h>


#endif // ASSEMBLER



#endif	// _ARM_TRAP_H_

