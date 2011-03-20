#if (!defined ASSEMBLER)

#include <phantom_types.h>

struct cpu_state_save
{
    u_int32_t           cpsr; // not saved!
    u_int32_t           ip;
    u_int32_t           sp;
    u_int32_t           lr;
    u_int32_t           fp;

    // exactly 4 or 8 ints above to guarantee 16-byte alignment
    
    // NB! Must be aligned at 16 byte boundary!
    //u_int8_t            fxstate[512];
};

#define STACK_PUSH(__sp,val) (*(--__sp) = (int)(val))


typedef struct cpu_state_save cpu_state_save_t;

#endif //ASSEMBLER


#define CSTATE_CPSR             0
#define CSTATE_IP               4
#define CSTATE_SP               8
#define CSTATE_LR              12
#define CSTATE_FP              16
//#define CSTATE_FXSTATE          20
