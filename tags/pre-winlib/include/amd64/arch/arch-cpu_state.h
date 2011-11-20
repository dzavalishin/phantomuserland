// temp off
#define FXSAVE 0


#if (!defined ASSEMBLER) && (!defined __ASSEMBLY__)

#include <phantom_types.h>

struct cpu_state_save
{
    register_t          flags; // not saved!
    register_t          rip;
    register_t          rsp;
    register_t          rbp;

    // exactly 4 or 8 ints above to guarantee 16-byte alignment

    // NB! Must be aligned at 16 byte boundary!
    u_int8_t            fxstate[512];
};

#define STACK_PUSH(rsp,val) (*(--rsp) = (register_t)(val))


typedef struct cpu_state_save cpu_state_save_t;

#endif //ASSEMBLER


#define CSTATE_FLAGS            0
#define CSTATE_EIP              8
#define CSTATE_ESP              16
#define CSTATE_EBP              24
#define CSTATE_FXSTATE          32

