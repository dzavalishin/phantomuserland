/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - painting boxes.
 *
**/

#include <video/window.h>
#include <video/internal.h>
#include <video/vops.h>



void w_fill_box( window_handle_t w, int x,int y,int lx,int ly, rgba_t c)
{
    int i;

    for(i=y;i<y+ly;i++)
        w_draw_line(w, x,i,x+lx-1,i,c);
}



void w_draw_box( window_handle_t  w, int x,int y,int lx, int ly, rgba_t c)
{
    w_draw_line(w,x,y,x+lx-1,y,c);
    w_draw_line(w,x,y+ly-1,x+lx-1,y+ly-1,c);
    w_draw_line(w,x,y,x,y+ly-1,c);
    w_draw_line(w,x+lx-1,y,x+lx-1,y+ly-1,c);
}


void w_draw_rect( window_handle_t win, rgba_t color, rect_t r )
{
    w_draw_box( win, r.x, r.y, r.xsize, r.ysize, color );
}

void w_fill_rect( window_handle_t w, rgba_t color, rect_t r )
{
    if( rect_win_bounds( &r, w ) )
        return;

    int yp = r.y + r.ysize - 1;
    for( ; yp >= r.y; yp-- )
    {
        rgba_t *dst = w->w_pixel + yp*w->xsize + r.x;
        rgba2rgba_replicate( dst, &color, r.xsize );
    }
}


void
w_fill( window_handle_t win, rgba_t color )
{
    int i = (win->xsize * win->ysize) - 1;
    rgba_t *dest = win->w_pixel;

#if defined(ARCH_ia32) && 1
    asm volatile(
                 "\
                 cld ; \
                 movl %0,%%eax; \
                 movl %1,%%ecx; \
                 movl %2,%%edi; \
                 rep stosl    ; \
                 "
                 : /* no outputs */
//                 : "g" (color), "g" (i), "g" (&(win->w_pixel))
                 : "g" (color), "g" (i), "g" (dest)
                 : "eax", "ecx", "edi"
                );
#else
    for( ; i >= 4;  )
    {
        win->pixel[i--] = color;
        win->pixel[i--] = color;
        win->pixel[i--] = color;
        win->pixel[i--] = color;
    }
    for( ; i >= 0; i-- )
        win->pixel[i] = color;
#endif
}
