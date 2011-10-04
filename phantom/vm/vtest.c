#include <phantom_libc.h>
//#include <drv_video_screen.h>

#include <video/screen.h>
#include <video/font.h>

#include "winhal.h"
#include "video/win_local.h"

#define WXS 240
#define WYS 160

void videotest(void)
{

    drv_video_window_t *w = drv_video_window_create( WXS, WYS, 400, 400, COLOR_BLACK, "Test Window" );

    _drv_video_winblt( w );
    drv_win_screen_update();
    getchar();



    drv_video_window_draw_line( w, 0, 0, WXS, WYS, COLOR_RED );
    drv_video_window_fill_ellipse( w, 30, 30, 15, 27, COLOR_BLUE );

    drv_video_font_draw_string( w, &drv_video_8x16san_font, "Test font", COLOR_BLACK, COLOR_GREEN, 0, 0 );

    drv_video_window_fill_box( w,  40, 32, 33, 10, COLOR_RED );


    _drv_video_winblt( w );
    drv_win_screen_update();
    getchar();

}

