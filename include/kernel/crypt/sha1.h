#ifndef __SHA1_H
#define __SHA1_H

//#include "types.h" // u32
#include <sys/types.h>

#define SHA1_HASH_SIZE 20

u_int32_t sha1( const u_int8_t *data, u_int32_t length, u_int8_t *hash );

#endif // sha1.h
