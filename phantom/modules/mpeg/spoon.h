#ifndef _spoon_h
#define _spoon_h


#include <sys/types.h>


#ifdef __cplusplus
extern "C" {
#endif

void spoon_putpixel( int x, int y, uint32 col );
void spoon_sync();

sint32 init_spoon( int width, int height );
void clean_spoon();

#ifdef __cplusplus
}
#endif


#endif

