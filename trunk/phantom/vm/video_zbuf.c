/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Z buffer implementation.
 *
 * Z buffer value 0 is lowest (most far) position. It is supposed to be used by background.
 *
**/

//#include <drv_video_screen.h>
#include <phantom_types.h>
#include <phantom_libc.h>
#include <assert.h>

//#include <video.h>
#include <video/rect.h>
#include <video/zbuf.h>
#include <video/screen.h>



/** Z buffer itself. */

zbuf_t *zbuf = 0;

static u_int32_t zbsize = 0;
static u_int32_t zbwidth = 0;

static rect_t zbuf_rect;

void video_zbuf_init()
{
    if(zbuf) free(zbuf);

    zbsize = get_screen_xsize() * get_screen_ysize() * sizeof(zbuf_t);
    zbwidth = get_screen_xsize();

    zbuf = malloc( zbsize );

    zbuf_rect.x = 0;
    zbuf_rect.y = 0;
    zbuf_rect.xsize = get_screen_xsize();
    zbuf_rect.ysize = get_screen_ysize();

    video_zbuf_reset();
}



void video_zbuf_reset()
{
    memset( zbuf, 0, zbsize );
}


void video_zbuf_reset_z(int z)
{
    memset( zbuf, z, zbsize );
}


/**
 *
 * Reset given square to 0 for window.
 *
**/

void video_zbuf_reset_win( window_handle_t w )
{
    // TODO XXX HACK alert - hardcoded decorations size
    const int bw = 3;
    video_zbuf_reset_square( w->x - bw, w->y - bw, w->xsize + bw*2, w->ysize + bw*2 + 21 );
}



/**
 *
 * Reset given square to given z position. Usually to zero.
 *
**/

void video_zbuf_reset_square(int x, int y, int xsize, int ysize )
{
    video_zbuf_reset_square_z( x, y, xsize, ysize, 0 );
}

static int zb_upside = 0;
void video_zbuf_turn_upside(int v) { zb_upside = v; }


void video_zbuf_reset_square_z(int x, int y, int xsize, int ysize, u_int8_t zpos )
{
    rect_t out;
    rect_t a;

    a.x = x;
    a.y = y;
    a.xsize = xsize;
    a.ysize = ysize;

    if(!rect_mul( &out, &a, &zbuf_rect ))
        return;

    int ys;
    for(ys = 0; ys < out.ysize; ys++)
    {
        int linpos;
        if( zb_upside )
            linpos = out.x + out.y * zbwidth;
        else
            linpos = out.x + ( (get_screen_ysize()-1) - out.y) * zbwidth;

        out.y++;

        void *p = zbuf+linpos;
        size_t len = out.xsize * sizeof(zbuf_t);

        assert( p >= (void*)zbuf );
        assert( p+len <= ((void*)zbuf)+zbsize );

        memset( p, zpos, len );
    }
}


/**
 *
 * Check if we shall draw this pixel.
 *
 * linpos is linear position in videobuffer (usually x+y*xsize).
 * zpos is current window's z coordinate.
 *
**/
int video_zbuf_check( int linpos, u_int8_t zpos )
{
    if( zbuf[linpos] > zpos ) return 0;
    if( zbuf[linpos] == zpos ) return 1;

    // Check only if we going to modify zbuf
    if( linpos > zbsize ) return 1;

    zbuf[linpos] = zpos;
    return 1;
}


void video_zbuf_dump()
{
    int y;
    int ysize = get_screen_ysize();
    int xsize = get_screen_xsize();

    printf("zbuf dump %d*%d\n", xsize, ysize );
    for(y = 0; y < ysize; y++)
    {
        int x;
        int xsize = xsize;

        for( x = 0; x < xsize; x++ )
        {
            int linpos = x + y*xsize;
            printf("%02X", zbuf[linpos] );
        }
        printf("\n");
    }
}


#include <video/screen.h>

void video_zbuf_paint()
{
    int np = get_screen_xsize() * get_screen_ysize();

#if 1
    if(get_screen_bpp() == 24)
    {
        zbuf_t *zbp = zbuf;
        rgb_t *d = (void *)video_drv->screen;
        while( np-- )
        {
            d->r = d->g = d->b = *zbp++;
            d++;
        }
    }
    if(get_screen_bpp() == 32)
#endif
    {
        zbuf_t *zbp = zbuf;
        rgba_t *d = (void *)video_drv->screen;
        while( np-- )
        {
            d->r = d->g = d->b = *zbp++;
            d->a = 0xFF;
            d++;
        }
    }
}



