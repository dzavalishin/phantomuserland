/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Screen getters/setters.
 *
 *
**/


#include <video/screen.h>
#include <video/vops.h>
#include "linestart.h"


int scr_get_xsize(void) { return video_drv->xsize; }
int scr_get_ysize(void) { return video_drv->ysize; }
int scr_get_bpp(void) { return video_drv->bpp; }





// movers. default to 24bpp
// void rgba2rgba_zbmove( struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos )
void (*bit_zbmover_to_screen)( void *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos ) = (void *)rgba2rgb_zbmove;
void (*bit_mover_to_screen)( void *dest, const struct rgba_t *src, int nelem ) = (void *)rgba2rgb_move;
void (*bit_mover_to_screen_noalpha)( void *dest, const struct rgba_t *src, int nelem ) = (void *)rgba2rgb_move_noalpha;

void (*bit_mover_from_screen)( struct rgba_t *dest, void *src, int nelem ) = (void *)rgb2rgba_move;
int      bit_mover_byte_step = 3;


void switch_screen_bitblt_to_32bpp( int use32bpp )
{
    if(use32bpp)
    {
        bit_zbmover_to_screen = (void *)rgba2rgba_zbmove;
        bit_mover_to_screen   = (void *)rgba2rgba_move;
        bit_mover_to_screen_noalpha = (void *)rgba2rgba_move_noalpha;
        bit_mover_from_screen = (void *)rgba2rgba_24_move;
#if defined(ARCH_ia32) && 0
        if(ARCH_HAS_FLAG(ARCH_IA32_SSE2))
        {
            bit_zbmover_to_screen = (void *)auto_rgba2rgba_zbmove;

            printf("use sse bitblt mover\n");
        }
#endif
        bit_mover_byte_step = 4;
        video_drv->bpp = 32;
    }
    else
    {
        bit_zbmover_to_screen = (void *)rgba2rgb_zbmove;
        bit_mover_to_screen   = (void *)rgba2rgb_move;
        bit_mover_to_screen_noalpha = (void *)rgba2rgb_move_noalpha;
        bit_mover_from_screen = (void *)rgb2rgba_move;

        bit_mover_byte_step = 3;
        video_drv->bpp = 24;
    }
}
