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


#include "gcc_replacements.h"

#include <stdarg.h>

#include <phantom_libc.h>
#include <kernel/boot.h>

#include "vm/root.h"
//#include "vm/bulk.h"

//#include "vm/internal_da.h"

#include "main.h"
#include "win_bulk.h"

#include "drv_video_screen.h"

#define MAXENVBUF 128
static char *envbuf[MAXENVBUF] = { 0 };












#define wysize 60
#define wxsize 80

static struct rgba_t win[wysize*wxsize];


static int size = 40*1024*1024;
static void *mem;


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
        int result = drv_video_ppm_load( &bmp, data );

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

#if VIDEO_ZBUF
        drv_video_bitblt(win, 0, i*10, wxsize, wysize, i & 1 ? 0xFF : 0x55 );
#else
        drv_video_bitblt(win, 0, i*10, wxsize, wysize );
#endif
        //drv_win_screen_update();
        //drv_video_update();
    }


    //drv_video_window_t *w = malloc(drv_video_window_bytes(140,80));
    //w->xsize = 140;
    //w->ysize = 80;

    drv_video_window_t *w = drv_video_window_create(140,80, 200, 200, COLOR_LIGHTGRAY );

    //drv_video_window_clear( w );
    //drv_video_window_fill( w, COLOR_LIGHTGRAY );
    drv_video_winblt( w );

#if VIDEO_ZBUF
    video_zbuf_reset();

    fillw( 0xFF, 0, 0 );
    drv_video_bitblt(win, 150, 100, wxsize, wysize, 0xEF );

    //video_zbuf_dump();

    fillw( 0, 0xFF, 0 );
    drv_video_bitblt(win, 175, 120, wxsize, wysize, 0xEE );
    drv_video_bitblt(win, 400, 400, wxsize, wysize, 0xEE );
#endif

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













struct drv_video_screen_t        *video_drv = 0;

extern int pvm_win_init(); // We need it only here

static void mouse_callback()
{
    //drv_video_bitblt(win, video_drv->mouse_x, video_drv->mouse_y, wxsize, wysize);
    //drv_video_update();

    video_drv->redraw_mouse_cursor();
}


static void args(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    printf("\n\n\n\n----- Phantom exec test v. 0.5\n\n");

    drv_video_win32.mouse = mouse_callback;
    video_drv = &drv_video_win32;

    args(argc,argv);

    pvm_bulk_init( bulk_seek_f, bulk_read_f );

    pvm_win_init();

    drv_video_set_mouse_cursor(drv_video_get_default_mouse_bmp());


    mem = malloc(size+1024*10);
    setDiffMem( mem, malloc(size+1024*10), size );

    hal_init( mem, size );

    videotest();

    getchar();

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

//ui_loop( argc, argv, "test");
    printf("\nPhantom code finished\n" );
    getchar();

#if 0
    pvm_memcheck();

    printf("will run GC\n" );
    run_gc();

    printf("press enter\n" );
//    getchar();

    pvm_memcheck();
#endif

    save_mem(mem, size);
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
    main_env = &envbuf;

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





