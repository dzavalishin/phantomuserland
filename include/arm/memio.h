#include <sys/types.h>

#define W32(__a,__v) ( *((u_int32_t*)(__a)) ) = __v
#define R32(__a) ( *((u_int32_t*)(__a)) )

#define W16(__a,__v) ( *((u_int16_t*)(__a)) ) = __v
#define R16(__a) ( *((u_int16_t*)(__a)) )

#define W8(__a,__v) ( *((u_int8_t*)(__a)) ) = __v
#define R8(__a) ( *((u_int8_t*)(__a)) )

