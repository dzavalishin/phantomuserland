#include <sys/types.h>

#define W32(__a,__v) ( *((volatile u_int32_t volatile  *)(__a)) ) = __v
#define R32(__a) ( *((volatile u_int32_t volatile *)(__a)) )

#define W16(__a,__v) ( *((volatile u_int16_t*)(__a)) ) = __v
#define R16(__a) ( *((volatile u_int16_t*)(__a)) )

#define W8(__a,__v) ( *((volatile u_int8_t*)(__a)) ) = __v
#define R8(__a) ( *((volatile u_int8_t*)(__a)) )

