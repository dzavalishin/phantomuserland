#ifndef CPU_STATE_H
#define CPU_STATE_H


// temp off
#define FXSAVE 0


#ifndef ASSEMBLER

#include <phantom_types.h>

struct cpu_state_save
{
    u_int32_t           flags; // not saved!
    u_int32_t           eip;
    u_int32_t           esp;
    u_int32_t           ebp;

    // these are just pushed in switch code
    //uint32_t            ebx;
    //uint32_t            esi;
    //uint32_t            edi;
    //uint32_t            cr2;

    // exactly 4 or 8 ints above to guarantee 16-byte alignment

    u_int8_t            fxstate[512];
};

//#define THREAD_PUSH(t,val) (*(--((int *)t->cpu.esp) ) = (int)(val))

#define STACK_PUSH(esp,val) (*(--esp) = (int)(val))


typedef struct cpu_state_save cpu_state_save_t;

#endif //ASSEMBLER


#define CSTATE_FLAGS            0
#define CSTATE_EIP		4
#define CSTATE_ESP              8
#define CSTATE_EBP		12
#define CSTATE_FXSTATE          16


#endif // CPU_STATE_H

