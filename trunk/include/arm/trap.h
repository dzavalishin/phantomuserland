
#ifndef	_ARM_TRAP_H_
#define	_ARM_TRAP_H_

#ifndef ARCH_arm
#warning Arm code! Wrong arch?
#endif

#ifndef GENERAL_TRAP_H
#warning include <kernel/trap.h> instead!
#endif


#define	T_RESET         	0
#define	T_INVALID_OPCODE	1		/* invalid op code */
#define	T_SOFT_INT              2		/* soft int instruction */
#define	T_PREFETCH_ABORT        3
#define	T_DATA_ABORT            4
#define	T_RESERVED              5
#define	T_IRQ			6
#define	T_FRQ                   7




#ifndef ASSEMBLER


/* This structure corresponds to the state of user registers
   as saved upon kernel trap/interrupt entry.
   As always, it is only a default implementation;
   a well-optimized microkernel will probably want to override it
   with something that allows better optimization.  */

struct trap_state {

    u_int32_t   r1;
    u_int32_t   r0;

    u_int32_t	trapno;

    // Processor state 
    u_int32_t   lr;
    u_int32_t   sp;
    u_int32_t   fp;
    u_int32_t   ip;
    u_int32_t   psr;

};

/* The actual trap_state frame pushed by the processor
   varies in size depending on where the trap came from.  */
//#define TR_KSIZE	((int)&((struct trap_state*)0)->esp)
//#define TR_V86SIZE	sizeof(struct trap_state)

#define ARCH_N_TRAPS 8


#else // ASSEMBLER

#include <arm/asm.h>


#endif // ASSEMBLER



#endif	// _ARM_TRAP_H_

