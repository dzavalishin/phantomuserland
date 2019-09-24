// temp off
#define FXSAVE 1


#if (!defined ASSEMBLER) && (!defined __ASSEMBLY__)

#include <phantom_types.h>
#include <kernel/config.h>


// NB! Don't touch? Accessed from asm by hardcoded offsets?
struct cpu_state_save
{
    u_int32_t           flags; // not saved!
    u_int32_t           eip;
    u_int32_t           esp;
    u_int32_t           ebp;
/*
#if CONF_DUAL_PAGEMAP
    u_int32_t           cr3;
    u_int32_t           _fill_1;
    u_int32_t           _fill_2;
    u_int32_t           _fill_3;
#endif
*/
    // these are just pushed in switch code
    //uint32_t            ebx;
    //uint32_t            esi;
    //uint32_t            edi;
    //uint32_t            cr2;

    // exactly 4 or 8 ints above to guarantee 16-byte alignment

	// NB! Must be aligned at 16 byte boundary!
    u_int8_t            fxstate[512];
};

#define STACK_PUSH(esp,val) (*(--esp) = (int)(val))


typedef struct cpu_state_save cpu_state_save_t;

#endif //ASSEMBLER


#define CSTATE_FLAGS            0
#define CSTATE_EIP		4
#define CSTATE_ESP              8
#define CSTATE_EBP		12
#define CSTATE_FXSTATE          16
