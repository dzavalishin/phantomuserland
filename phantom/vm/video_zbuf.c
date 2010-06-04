#include <drv_video_screen.h>
#include <phantom_types.h>
#include <phantom_libc.h>

#include <video/rect.h>

/**
 *
 * Z buffer implementation.
 *
 * Z buffer value 0 is lowest (most far) position. It is supposed to be used by background.
 *
**/


/** Z buffer itself. */

u_int8_t *zbuf = 0;

static u_int32_t zbsize = 0;
static u_int32_t zbwidth = 0;

static rect_t zbuf_rect;

void video_zbuf_init()
{
    if(zbuf) free(zbuf);

    zbsize = video_drv->xsize * video_drv->ysize;
    zbwidth = video_drv->xsize;

    zbuf = malloc( zbsize );

    zbuf_rect.x = 0;
    zbuf_rect.y = 0;
    zbuf_rect.xsize = video_drv->xsize;
    zbuf_rect.ysize = video_drv->ysize;

    video_zbuf_reset();
}



void video_zbuf_reset()
{
    memset( zbuf, 0, zbsize );
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
    for(ys = 0; ys < ysize; ys++)
    {
        int linpos = out.x + out.y*zbwidth;
        out.y++;
        memset( zbuf+linpos, zpos, out.xsize );
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
    int ysize = video_drv->ysize;

    printf("zbuf dump %d*%d\n", video_drv->xsize, video_drv->ysize );
    for(y = 0; y < ysize; y++)
    {
        int x;
        int xsize = video_drv->xsize;

        for( x = 0; x < xsize; x++ )
        {
            int linpos = x + y*xsize;
            printf("%02X", zbuf[linpos] );
        }
        printf("\n");
    }
}


