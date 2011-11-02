#if (!defined ASSEMBLER)

#include <phantom_types.h>

struct cpu_state_save
{
    u_int32_t           gp;
    u_int32_t           sp;
    u_int32_t           fp;
    u_int32_t           ra; // == pc

    u_int32_t           status; // NB! Not really restored!

    u_int32_t           u0;
    u_int32_t           u1;
    u_int32_t           u2;

    // exactly 4 or 8 ints above to guarantee 16-byte alignment
    
    // NB! Must be aligned at 16 byte boundary!
    //u_int8_t            fxstate[512];
};

#define STACK_PUSH(__sp,val) (*(--__sp) = (int)(val))


typedef struct cpu_state_save cpu_state_save_t;

#endif //ASSEMBLER


#define CSTATE_GP               0
#define CSTATE_SP               4
#define CSTATE_FP               8
#define CSTATE_RA              12
#define CSTATE_STATUS          16
//#define CSTATE_FXSTATE          20
