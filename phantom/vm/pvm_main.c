/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: no
 * Preliminary: yes
 *
 * This source file implements Windows based wrapper for VM to
 * run in Windows-hosted environment.
 *
**/

/**
 *
 * Virtual machine test starter.
 *
**/


//#include "gcc_replacements.h"

#include <stdarg.h>

#include <phantom_libc.h>
#include <kernel/boot.h>
#include <kernel/init.h>
#include <kernel/debug.h>

#include <vm/root.h>
//#include "vm/bulk.h"
//#include "vm/internal_da.h"

#include <hal.h>
#include "main.h"
#include "win_bulk.h"

//#include <drv_video_screen.h>
#include <video/screen.h>
#include <video/internal.h>

#define MAXENVBUF 128
static char *envbuf[MAXENVBUF] = { 0 };



#if 0

static const int MaxColor     = 110;


static struct rgba_t palette[256];

static void mkFlamePalette()
{
    u_int8_t i;

    memset( palette, sizeof(palette), 0 );

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
        struct rgba_t *dest = w->pixel;

        int i;
        for( i = 0; i < vsize; i++ )
        {
#if 1
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


    //drv_video_window_t *w = malloc(drv_video_window_bytes(140,80));
    //w->xsize = 140;
    //w->ysize = 80;

    drv_video_window_t *w = drv_video_window_create( 140,80, 200, 200, COLOR_LIGHTGRAY, "Test Window" );

    //drv_video_window_clear( w );
    //drv_video_window_fill( w, COLOR_LIGHTGRAY );
    drv_video_winblt( w );

    //flame(w);

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
}





#endif



static int size = 220*1024*1024;
static void *mem;




struct drv_video_screen_t        *video_drv = 0;

extern int pvm_video_init(); // We need it only here

static void mouse_callback()
{
    //drv_video_bitblt(win, video_drv->mouse_x, video_drv->mouse_y, wxsize, wysize);
    //drv_video_update();

    video_drv->mouse_redraw_cursor();
}


static void args(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    printf("\n\n\n\n----- Phantom exec test v. 0.5\n\n");


    run_init_functions( INIT_LEVEL_PREPARE );
    run_init_functions( INIT_LEVEL_INIT ); // before video

    //drv_video_win32.mouse = mouse_callback;
    //video_drv = &drv_video_win32;
    //video_drv = &drv_video_x11;

    args(argc,argv);

    pvm_bulk_init( bulk_seek_f, bulk_read_f );

    pvm_video_init();
    video_drv->mouse = mouse_callback;

    drv_video_init_windows();
    init_main_event_q();
    init_new_windows();

    scr_mouse_set_cursor(drv_video_get_default_mouse_bmp());


    mem = malloc(size+1024*10);
    setDiffMem( mem, malloc(size+1024*10), size );

    hal_init( mem, size );
    //pvm_alloc_threaded_init(); // no threads yet - no lock

    run_init_functions( INIT_LEVEL_LATE );

#if 0
    videotest();
    //getchar();
    exit(0);
#endif

#if 0
    new_videotest();
    getchar();
    exit(0);
#endif


    char *dir = getenv("PHANTOM_HOME");
    char *rest = "plib/bin/classes";

    if( dir == NULL )
    {
        dir = "pcode";
        rest = "classes";
    }

    char fn[1024];
    snprintf( fn, 1024, "%s/%s", dir, rest );

    if( load_code( &bulk_code, &bulk_size, fn ) ) //"pcode/classes") )
    {
        printf("No bulk classes file '%s'\n", fn );
        exit(22);
    }
    bulk_read_pos = bulk_code;


    pvm_root_init();


    // TODO use stray catcher in pvm_test too
    //stray();

#if 0
//ui_loop( argc, argv, "test");
    printf("\nPhantom code finished\n" );
    //getchar();
    //{ char c; read( 0, &c, 1 ); }
	sleep(100);
#else
    dbg_init();
    kernel_debugger();
#endif

#if 0
    pvm_memcheck();

    printf("will run GC\n" );
    run_gc();

    printf("press enter\n" );
//    getchar();

    pvm_memcheck();

    save_mem(mem, size);
#endif
    return 0;
}




static void usage()
{
    printf(
           "Usage: pvm_test [-flags]\n\n"
           "Flags:\n"
           "-di\t- debug (print) instructions\n"
           "-h\t- print this\n"
           );
}

extern int debug_print_instr;

int main_envc;
const char **main_env;


static void args(int argc, char* argv[])
{
    main_envc = 0;
    main_env = (const char **) &envbuf;

    while(argc-- > 1)
    {
        char *arg = *++argv;

        if( *arg != '-' && index( arg, '=' ) )
        {
            if( main_envc >= MAXENVBUF )
            {
                printf("Env too big\n");
                exit(22);
            }
            envbuf[main_envc++] = arg;
            envbuf[main_envc] = 0;
            continue;
        }

        if( *arg != '-' )
        {
            usage(); exit(22);
        }
        arg++; // skip '-'

        switch( *arg )
        {
        case 'd':
            {
                char c;
                arg++;
                while( (c = *arg++ ) != 0 )
                {
                    switch(c) {
                    case 'i': debug_print_instr = 1; break;

                    }
                }
            }
            break;

        case 'h':
        default:
            usage(); exit(22);
        }
    }

}


void phantom_debug_register_stray_catch( void *a, int s, const char*n )
{
    // Ignore
    (void) a;
    (void) s;
    (void) n;
}



