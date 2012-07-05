/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Wrappers around bitblt workers. These wrappers are used by video drivers.
 *
**/

#include <video/screen.h>

// Placeholder for absent drv methods
void vid_null() {}




void win2blt_flags( u_int32_t *flags, const window_handle_t w )
{
    *flags = 0;

#if VIDEO_NOZBUF_BLIT
    // If window is not covered, we can ignore z buffer when painting
    if( w->state & WSTATE_WIN_UNCOVERED )
        (*flags) |= BLT_FLAG_NOZBUF;
#endif

    // If window does not use alpha, we can ignore alpha byte
    if( !(w->flags & WFLAG_WIN_NOTOPAQUE) )
        (*flags) |= BLT_FLAG_NOALPHA;
}





void vid_bitblt_part_forw(const rgba_t *from, int src_xsize, int src_ysize, int src_xpos, int src_ypos, int dst_xpos, int dst_ypos, int xsize, int ysize, zbuf_t zpos, u_int32_t flags )
{
    vid_bitblt_part(from, src_xsize, src_ysize, src_xpos, src_ypos, dst_xpos, dst_ypos, xsize, ysize, 0, zpos, flags );
}

void vid_bitblt_part_rev(const rgba_t *from, int src_xsize, int src_ysize, int src_xpos, int src_ypos, int dst_xpos, int dst_ypos, int xsize, int ysize, zbuf_t zpos, u_int32_t flags )
{
    vid_bitblt_part(from, src_xsize, src_ysize, src_xpos, src_ypos, dst_xpos, dst_ypos, xsize, ysize, 1, zpos, flags );
}




void vid_bitblt_forw(const struct rgba_t *from, int xpos, int ypos, int xsize, int ysize, zbuf_t zpos, u_int32_t flags  )
{
    vid_bitblt_worker( from, xpos, ypos, xsize, ysize, 0, zpos, flags );
}


void vid_bitblt_rev(const struct rgba_t *from, int xpos, int ypos, int xsize, int ysize, zbuf_t zpos, u_int32_t flags )
{
    vid_bitblt_worker( from, xpos, ypos, xsize, ysize, 1, zpos, flags );
}



#if VIDEO_DRV_WINBLT
void 	vid_win_winblt(const window_handle_t from, int xpos, int ypos, zbuf_t zpos )
{
    u_int32_t flags = 0;
    win2blt_flags( &flags, from );
    vid_bitblt_worker( from->r_pixel, xpos, ypos, from->xsize, from->ysize, 0, zpos, flags );
}

void 	vid_win_winblt_rev(const window_handle_t from, int xpos, int ypos, zbuf_t zpos )
{
    u_int32_t flags = 0;
    win2blt_flags( &flags, from );
    vid_bitblt_worker( from->r_pixel, xpos, ypos, from->xsize, from->ysize, 1, zpos, flags );
}
#endif






void vid_readblt_forw( struct rgba_t *to, int xpos, int ypos, int xsize, int ysize )
{
    vid_bitblt_reader( to, xpos, ypos, xsize, ysize, 0 );
}


void vid_readblt_rev( struct rgba_t *to, int xpos, int ypos, int xsize, int ysize )
{
    vid_bitblt_reader( to, xpos, ypos, xsize, ysize, 1 );
}


