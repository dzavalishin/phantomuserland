#include <phantom_libc.h>
//#include <drv_video_screen.h>

#include <video/screen.h>
#include <video/font.h>

#include "winhal.h"
//#include "video/win_local.h"

#define WXS (240*2)
#define WYS (160*2)

void videotest(void)
{
    drv_video_window_t *w1 = drv_video_window_create( WXS, WYS, 350, 350, COLOR_BLACK, "Test 2", WFLAG_WIN_DECORATED );

    drv_video_window_t *w = drv_video_window_create( WXS, WYS, 300, 300, COLOR_BLACK, "Test Window", WFLAG_WIN_DECORATED );

    drv_video_winblt( w );
    drv_win_screen_update();
    getchar();



    w_draw_line( w, 0, 0, WXS, WYS, COLOR_RED );
    w_fill_ellipse( w, 30, 30, 15, 27, COLOR_BLUE );

    w_font_draw_string( w, &drv_video_8x16san_font, "Test font", COLOR_BLACK, COLOR_GREEN, 0, 0 );

    w_fill_box( w,  40, 32, 33, 10, COLOR_RED );


    drv_video_winblt( w );
    drv_win_screen_update();
    getchar();

}

