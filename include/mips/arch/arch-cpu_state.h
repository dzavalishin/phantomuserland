#if (!defined ASSEMBLER)

#include <phantom_types.h>

struct cpu_state_save
{
    u_int32_t           sp;
    u_int32_t           lr;
    u_int32_t           fp;
    u_int32_t           sr; // cp0 r12 - status

    // exactly 4 or 8 ints above to guarantee 16-byte alignment
    
    // NB! Must be aligned at 16 byte boundary!
    //u_int8_t            fxstate[512];
};

#define STACK_PUSH(__sp,val) (*(--__sp) = (int)(val))


typedef struct cpu_state_save cpu_state_save_t;

#endif //ASSEMBLER


#define CSTATE_SP               0
#define CSTATE_LR               4
#define CSTATE_FP               8
#define CSTATE_SR              12
//#define CSTATE_FXSTATE          20
