#ifndef BITMAP_H
#define BITMAP_H

#include <video/color.h>

typedef struct drv_video_bitmap
{
    int         	xsize;
    int 		ysize;
    rgba_t              pixel[];
} drv_video_bitmap_t;

#endif //BITMAP_H
