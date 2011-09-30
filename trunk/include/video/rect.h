#ifndef VIDEO_RECT_H
#define VIDEO_RECT_H

typedef struct rect
{
    int x, y;
    int xsize, ysize;
} rect_t;

void rect_add( rect_t *out, rect_t *a, rect_t *b );
int rect_mul( rect_t *out, rect_t *a, rect_t *b );

int point_in_rect( int x, int y, rect_t *r );


#endif // VIDEO_RECT_H
