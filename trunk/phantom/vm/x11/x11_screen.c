/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: no
 *
 * This source file implements X11 based wrapper for VM to
 * run in Unix-hosted environment.
 *
 * TODO: GBR -> RGB!!
 *
**/

//#include <phantom_libc.h>

#include <string.h>
#include <assert.h>

#include <drv_video_screen.h>

#include "../gcc_replacements.h"

#include "screen_x11.h"

#define VSCREEN_WIDTH 1024
#define VSCREEN_HEIGHT 768


struct drv_video_screen_t        drv_video_x11 =
{
    "X11",
    // size
    0, 0,
    // mouse x y flags
    0, 0, 0,

    // screen
    0,

probe: (void *)drv_video_null,
start: (void *)drv_video_null,
stop:  (void *)drv_video_null,

    drv_video_null,
    (void*)drv_video_null,
    (void*)drv_video_null,

    drv_video_null,

mouse:    		drv_video_null,

redraw_mouse_cursor: 	drv_video_draw_mouse_deflt,
set_mouse_cursor: 	drv_video_set_mouse_cursor_deflt,
mouse_disable:          drv_video_mouse_off_deflt,
mouse_enable:          	drv_video_mouse_on_deflt,

};





static int eline = 0;


static int init_ok = 0;
static int init_err = 0;


static char screen_image [VSCREEN_WIDTH*VSCREEN_HEIGHT*3];



//static
void drv_x11_screen_update(void)
{
    win_x11_update(screen_image);
    eline = 1;
}

/*
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
    case WM_PAINT:

        break;


    case WM_MOUSEMOVE:
        {
            int xPos = (short)(0x0FFFF & lParam);//GET_X_LPARAM(lParam);
            int yPos = VSCREEN_HEIGHT - (short)(0x0FFFF & (lParam>>16));//GET_Y_LPARAM(lParam);

            //	printf("%d,%d\n", xPos, yPos );

            drv_video_x11.mouse_x = xPos;
            drv_video_x11.mouse_y = yPos;
            drv_video_x11.mouse_flags = wParam;
            drv_video_x11.mouse();

        }
        break;

    default:
        return DefWindowProc(hWnd, iMessage, wParam, lParam);
    }
    return 0;
}
*/


//int WINAPI
void    pvm_x11_window_thread()
{

    if(win_x11_init(VSCREEN_WIDTH,VSCREEN_HEIGHT))
    {
        init_err = 1;
        return; // BUG: report error
    }


    int i;
    for( i = 0; i < VSCREEN_WIDTH * VSCREEN_HEIGHT * 3; i++)
    {
        screen_image[i] = 34;
    }

    drv_video_x11.screen = screen_image;
    drv_video_x11.xsize = VSCREEN_WIDTH;
    drv_video_x11.ysize = VSCREEN_HEIGHT;
    drv_video_x11.update = &drv_x11_screen_update;
#if 1
    drv_video_x11.bitblt = &drv_video_bitblt_forw;
    drv_video_x11.winblt = &drv_video_x11_winblt;
    drv_video_x11.readblt = &drv_video_readblt_forw;
#else
    drv_video_x11.bitblt = &drv_video_bitblt_rev;
    drv_video_x11.winblt = &drv_video_x11_winblt_rev;
    drv_video_x11.readblt = &drv_video_readblt_rev;
#endif

    drv_video_x11.redraw_mouse_cursor = &drv_video_draw_mouse_deflt;
    drv_video_x11.set_mouse_cursor = &drv_video_set_mouse_cursor_deflt;
    drv_video_x11.mouse_disable = &drv_video_mouse_off_deflt;
    drv_video_x11.mouse_enable = &drv_video_mouse_on_deflt;

    init_ok = 1;


    win_x11_message_loop();
    //printf("Message loop end\n");

}


int pvm_x11_init()
{
    drv_video_x11.screen = 0; // Not ready yet

    printf("Starting X11 graphics 'driver'\n" );


    static unsigned long tid;
    if( 0 == CreateThread( 0, 0, (void *)&pvm_x11_window_thread, (void*)0, 0, &tid ) )
        panic("can't start window thread");


    int repeat = 10000;
    while(repeat-- > 0)
    {
        sleep(1);
        if( init_err ) break;
        if( init_ok )
        {
#if VIDEO_ZBUF
            video_zbuf_init();
#endif
            return 0;
        }

    }


    return -1;
}






