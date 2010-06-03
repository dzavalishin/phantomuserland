#ifndef VIDEO_RECT_H
#define VIDEO_RECT_H

typedef struct rect
{
    int x, y;
    int xsize, ysize;
} rect_t;

void rect_add( rect_t *out, rect_t *a, rect_t *b );
int rect_mul( rect_t *out, rect_t *a, rect_t *b );


#endif // VIDEO_RECT_H
