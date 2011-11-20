#include <sys/types.h>
#include <errno.h>

errno_t arm_id( addr_t base, u_int32_t part_num, u_int32_t cellId, int verbose );
errno_t arm_id_shift( addr_t base, int shift, u_int32_t part_num, u_int32_t cellId, int verbose );


