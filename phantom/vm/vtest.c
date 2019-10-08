#if 0 // moved to video_test.c

#define DEBUG_MSG_PREFIX "vm.vtest"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>

#include <video/screen.h>
#include <video/font.h>

#include "winhal.h"

#define WXS (240*2)
#define WYS (160*2)

void videotest(void)
{
    //printf("\n\nTEST\n\n");
    //SHOW_FLOW0( 1, "videotest()" );
    //drv_video_window_t *w1 = drv_video_window_create( WXS, WYS, 350, 350, COLOR_BLACK, "Test 2", WFLAG_WIN_DECORATED );

    drv_video_window_t *w = drv_video_window_create( WXS, WYS, 300, 300, COLOR_BLACK, "Test Window", WFLAG_WIN_DECORATED );

    drv_video_winblt( w );
    //win_scr_screen_update();

    //(void) getchar();

    //drv_video_winblt( w1 );

    //w_fill( w, (rgba_t){ 0, 0, 0, 0 } );
    //w_fill( w, (rgba_t){ 15, 15, 15, 0xFF } );
    //w_fill( w, (rgba_t){ 65, 65, 65, 0xFF } );
    //w_fill( w, (rgba_t){ 165, 165, 165, 0xFF } );

    w_draw_line( w, 0, 0, WXS, WYS, COLOR_RED );
    w_fill_ellipse( w, 30, 30, 15, 27, COLOR_BLUE );

    w_font_draw_string( w, &drv_video_8x16san_font, "Test font", COLOR_BLACK, COLOR_GREEN, 0, 0 );

    w_fill_box( w,  40, 32, 33, 10, COLOR_RED );

    /*
    font_handle_t font1 = w_get_tt_font_file( "OpenSans-Regular.ttf", 100 );

    if( font1 == INVALID_POOL_HANDLE )
    {
        printf("\n\nTTF 1 FAIL\n\n");
    }
    else
        w_ttfont_draw_string( w, font1,
                          "TrueType rulez рулит",
                          COLOR_LIGHTRED, COLOR_BLACK,
                          10, 80 );
    */

    font_handle_t font2 = w_get_system_font_ext( 50 );

    if( font2 == INVALID_POOL_HANDLE )
    {
        //w_font_draw_string( w, &drv_video_8x16san_font, "TrueType Font Failed", COLOR_BLACK, COLOR_GREEN, 0, 30 );
        printf("\n\nTTF 2 FAIL\n\n");
    }
    else
        w_ttfont_draw_string( w, font2,
                          "TrueType рулит",
                          COLOR_LIGHTRED, //COLOR_BLACK,
                          10, 50 );


    w_release_tt_font( font2 );

    drv_video_winblt( w );
    //win_scr_screen_update();

    (void) getchar();

}



#if 0


#define FLAME 1


#if FLAME

static const int MaxColor     = 110;


static struct rgba_t palette[256];

static void mkFlamePalette()
{
    u_int8_t i;

    memset( palette, 0, sizeof(palette) );

    for( i = 0; i < MaxColor; i++ )
        palette[i] = Hsi2Rgb(4.6-1.5*i/MaxColor,i/MaxColor,i/MaxColor);

    for( i = MaxColor; i < 255; i++ )
    {
        palette[i]=palette[i-1];

        if(palette[i].r < 63) palette[i].r++;
        if(palette[i].r < 63) palette[i].r++;

        if( ((i % 2) == 0) && (palette[i].g < 53) ) palette[i].g++;
        if( ((i % 2) == 0) && (palette[i].b < 63) ) palette[i].b++;
    }

}


void genflame( char *video, int vsize, int xsize )
{
    char *src = video+vsize-xsize;
    char *dst = video+vsize;
    int count = vsize-xsize;

    while(count--)
    {
        char acc = *src--;

        acc += src[0];
        acc += src[-319];
        acc += src[2];
        acc <<= 2;

        //acc &= 0xF;
        acc += 0x10;

        *dst-- = acc;
    }

    count = xsize;
    //dst = video + vsize-xsize;
    dst = video;
    while(count--)
    {
        *dst++ = random();
    }
}



// Just for fun :)
void flame(drv_video_window_t *w)
{
    int vsize = w->xsize * w->ysize;
    char *video = calloc( 1, vsize );

    mkFlamePalette();

    while(1)
    {
        genflame( video, vsize, w->xsize );

        char *src = video;
        struct rgba_t *dest = w->pixels;

        int i;
        for( i = 0; i < vsize; i++ )
        {
#if 0
            *dest++ = palette[*src++ * 3];
#else
            dest->a = 0xFF;
            dest->r = *src;
            dest->g = 0;
            dest->b = 0;

            src++;
            dest++;
#endif
        }

        drv_video_winblt( w );
    }


}










#define wysize 60
#define wxsize 80

static struct rgba_t win[wysize*wxsize];




static void fillw(char r, char g, char b )
{
    // fill
    int c;
    for( c = 0; c < wxsize*wysize; c++ )
    {
        win[c].r = r;
        win[c].g = g;
        win[c].b = b;
        win[c].a = 1;
    }
}



void *bmp_pixels;
int bmpxs, bmpys;

void videotest()
{

    //int ysize = 60;
    //int xsize = 80;
    //char win[ysize*xsize*3];

#if 0
    const char *bpm = "../../plib/resources/backgrounds/phantom_dz_large.pbm";

    {
        FILE *f = fopen(bpm, "r" );
        if(f == NULL )
        {
            printf("can't open %s\n", bpm );
            exit(33);
        }

        fseek( f, 0, SEEK_END );
        long size = ftell( f );
        fseek( f, 0, SEEK_SET );

        printf("Size of %s is %ld\n", bpm, size );

        char *data = malloc(size);

        fread( data, size, 1, f );
        fclose(f);

        struct drv_video_bitmap_t *bmp;
        int result = bmp_ppm_load( &bmp, data );

        free(data);

        if(result)
            printf("can't parse %s: %d\n", bpm, result );
        else
        {
            drv_video_bitblt(bmp->pixel, 0, 0, bmp->xsize, bmp->ysize, 0xFF );
        }

        bmp_pixels = bmp->pixel;
        bmpxs = bmp->xsize;
        bmpys = bmp->ysize;


    }
#endif

#if 0
    int i;

    for( i = 0; i < 10; i++ )
    {
        fillw( (i+2)*10, (i+2)*20, (i+4)*10 );

//#if VIDEO_ZBUF
        drv_video_bitblt(win, 0, i*10, wxsize, wysize, i & 1 ? 0xFF : 0x55 );
//#else
//        drv_video_bitblt(win, 0, i*10, wxsize, wysize );
//#endif
        //drv_win_screen_update();
        //drv_video_update();
    }
#endif

    //drv_video_window_t *w = malloc(drv_video_window_bytes(140,80));
    //w->xsize = 140;
    //w->ysize = 80;

    drv_video_window_t *w = drv_video_window_create( 140,80, 200, 200, COLOR_LIGHTGRAY, "Test Window", 0 );

    //drv_video_window_clear( w );
    //drv_video_window_fill( w, COLOR_LIGHTGRAY );
    drv_video_winblt( w );

#if FLAME
    flame(w);
#endif

#if 0
    drv_video_window_draw_line( w, 1, 1, 110, 20, COLOR_RED );
    drv_video_window_draw_line( w, 110, 1, 110, 20, COLOR_RED );
    drv_video_window_draw_line( w, 1, 20, 110, 20, COLOR_GREEN );

    drv_video_window_fill_ellipse( w, 30, 30, 15, 27, COLOR_BLUE );

    drv_video_window_fill_box( w,  40, 32, 33, 10, COLOR_RED );

    drv_video_window_draw_box( w, 45, 37, 33, 12, COLOR_BLACK );

    drv_video_winblt( w );

    getchar();

//#if VIDEO_ZBUF
    video_zbuf_reset();

    fillw( 0xFF, 0, 0 );
    drv_video_bitblt(win, 550, 100, wxsize, wysize, 0xEF );

    //video_zbuf_dump();

    fillw( 0, 0xFF, 0 );
    drv_video_bitblt(win, 575, 120, wxsize, wysize, 0xEE );
    drv_video_bitblt(win, 700, 400, wxsize, wysize, 0xEE );
//#endif
    video_zbuf_reset();

//getchar();

#if 1
    drv_video_font_scroll_line( w, &drv_video_8x16san_font, COLOR_DARKGRAY );
    drv_video_font_draw_string( w, &drv_video_8x16san_font, "Test font", COLOR_BLACK, 0, 0 );
    drv_video_font_scroll_line( w, &drv_video_8x16san_font, COLOR_LIGHTGRAY );
    drv_video_font_draw_string( w, &drv_video_8x16san_font, "Line 2 ok", COLOR_BLACK, 0, 0 );
    drv_video_font_scroll_line( w, &drv_video_8x16san_font, COLOR_DARKGRAY );
    drv_video_font_draw_string( w, &drv_video_8x16san_font, "GIMME THAT", COLOR_BLACK, 0, 0 );
#else
    int ttx = 0, tty = 0;
    drv_video_font_tty_string( w, &drv_video_8x16san_font,
                               "Just a little bit long text\nFrom a new line",
                               //"Just a little bit long ",
                               COLOR_BLACK, COLOR_LIGHTGRAY,
                               &ttx, &tty );
#endif
    drv_video_winblt( w );
    //getchar();

#endif
}





#endif



#endif


#endif

