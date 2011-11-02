
#ifndef	_ARM_TRAP_H_
#define	_ARM_TRAP_H_

#ifndef ARCH_arm
#warning Arm code! Wrong arch?
#endif

#ifndef GENERAL_TRAP_H
#warning include <kernel/trap.h> instead!
#endif


#define	T_RESET                 0
#define	T_INVALID_OPCODE        1		/* invalid op code */
#define	T_SOFT_INT              2		/* soft int instruction */
#define	T_PREFETCH_ABORT        3
#define	T_DATA_ABORT            4
#define	T_RESERVED              5
#define	T_IRQ                   6
#define	T_FRQ                   7




#ifndef ASSEMBLER


/* This structure corresponds to the state of user registers
   as saved upon kernel trap/interrupt entry.
   As always, it is only a default implementation;
   a well-optimized microkernel will probably want to override it
   with something that allows better optimization.  */

//#warning sync this with intr/trap asm entry code
/*
struct trap_state
{
	unsigned int r0;
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


	unsigned int usr_sp;
	unsigned int usr_lr;

        unsigned int trapno;
	unsigned int intno;

        //unsigned int svc_sp;
	//unsigned int svc_lr;

        unsigned int pc;
        unsigned int spsr;
};
*/


#ifdef ASSEMBLER
#define TRAP_STATE_TRAPNO	64
#define TRAP_STATE_INTNO	68
#define TRAP_STATE_PC		80
#endif







// Push from bottom of struct (upper addresses) to top

struct trap_state
{
    unsigned int r0;    // arg0, retval
    unsigned int r1;    // arg1
    unsigned int r2;    // arg2
    unsigned int r3;    // arg3
    unsigned int r4;
    unsigned int r5;
    unsigned int r6;
    unsigned int r7;
    unsigned int r8;
    unsigned int r9;
    unsigned int r10;
    unsigned int r11;
    unsigned int r12;


    unsigned int usr_sp;
    unsigned int usr_lr;

    unsigned int trapno;
    unsigned int intno;     // swi/svc instr code

    unsigned int pc;        // LR we got
    unsigned int spsr;      // Interrupted code's psr

    //unsigned int sys_sp;
    //unsigned int sys_lr;    // LR of system state we switched to
};



#define TS_PROGRAM_COUNTER pc




/* The actual trap_state frame pushed by the processor
   varies in size depending on where the trap came from.  */
//#define TR_KSIZE	((int)&((struct trap_state*)0)->esp)
//#define TR_V86SIZE	sizeof(struct trap_state)

#define ARCH_N_TRAPS 8


#else // ASSEMBLER

#include <arm/asm.h>


#endif // ASSEMBLER



#endif	// _ARM_TRAP_H_

