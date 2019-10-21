/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * This source file implements Windows based wrapper for VM to
 * run in Windows-hosted environment.
 *
 * Unix port of this wrapper is partially ready.
 *
**/


#define DEBUG_MSG_PREFIX "vm.main"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <stdarg.h>

#include <phantom_libc.h>
#include <kernel/boot.h>
#include <kernel/init.h>
#include <kernel/debug.h>
#include <kernel/json.h>
#include <kernel/net.h>

#include <vm/root.h>
#include <vm/internal_da.h>
#include <vm/p2c.h>
#include <vm/json.h>

#include <hal.h>
#include "main.h"
#include "win_bulk.h"
#include "winhal.h"

#include <video/screen.h>
#include <video/internal.h>


/*
void json_process_for_parent( const char *content, jsmntok_t *tokens, size_t n_tokens, int parent, pvm_object_t dir )
{
    //struct data_area_4_directory *tlda = pvm_object_da( pvm_root.class_dir, directory );
    // errno_t rc = hdir_add( tlda, name, name_len, content );
    
    printf("\n\nfor parent %d:\n", parent );

    int i;
    for( i = 0; i < n_tokens; i++ )
    {
        if( tokens[i].parent != parent ) continue;

        int size = tokens[i].end - tokens[i].start;
        printf("%d:\t parent %4d size %3d type %d", i, tokens[i].parent, size, tokens[i].type );
        if(size) printf(" '%.*s'", size, content + tokens[i].start );
        printf("\n");

        pvm_object_t subdir = 0;
        json_process_for_parent( content, tokens, n_tokens, tokens[i].parent, subdir );
    }

}
*/




void test_json()
{
#if 0
    //int net_curl( const char *url, char *obuf, size_t obufsize, const char *headers );
    static char buf[1024 * 50];
    net_curl( "http://api.weather.yandex.ru/v1/forecast?extra=true&limit=1", buf, sizeof buf, "X-Yandex-API-Key: 7bdab0b4-2d21-4a51-9def-27793258d55d\r\n" );

    //jsmntok_t *tokens;
    //size_t o_count = 0;


    //printf("buf '%s'\n", buf );

    const char *content = http_skip_header( buf );

    //printf("content '%s'\n\n", content );
#if 1
/*
    json_value *jv = json_parse( content, strlen(content) );
    pvm_object_t top = pvm_convert_json_to_objects( jv );
    json_value_free( jv );
    printf("\n\n------------------------- OBJECT LAND -------------------\n\n" );
*/
    pvm_object_t top = pvm_json_parse( content );
    pvm_print_json( top );

#else
    errno_t rc = json_parse( content, &tokens, &o_count );
    if( rc )
    {
        printf("err json %d\n", rc );
        return;
    }

    {
        int i;
        for( i = 0; i < o_count; i++ )
        {
            //printf("%d:\t parent %3d '%.*s'\n", i, tokens[i].parent, buf+tokens[i].start, tokens[i].size );
            int size = tokens[i].end - tokens[i].start;
            printf("%d:\t parent %4d size %3d type %d", i, tokens[i].parent, size, tokens[i].type );
            if(size)
                printf(" '%.*s'", size, content + tokens[i].start );
            printf("\n");
        }
    }


    json_process_for_parent( content, tokens, o_count, -1, tld );

#endif
#endif
}





hal_mutex_t  snap_interlock_mutex;  // from snap_sync.c
hal_cond_t   vm_thread_wakeup_cond; // from snap_sync.c


#define MAXENVBUF 128
static char *envbuf[MAXENVBUF] = { 0 };

static int arg_run_debugger = 0;



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
    printf("\n\n\n\n----- Phantom exec test v. 0.6\n\n");

    hal_mutex_init( &snap_interlock_mutex, "snap_interlock_mutex" );
    hal_cond_init( &vm_thread_wakeup_cond, "vm_thread_wakeup_cond" );

    run_init_functions( INIT_LEVEL_PREPARE );
    run_init_functions( INIT_LEVEL_INIT ); // before video

    //drv_video_win32.mouse = mouse_callback;
    //video_drv = &drv_video_win32;
    //video_drv = &drv_video_x11;

    args(argc,argv);

    pvm_bulk_init( bulk_seek_f, bulk_read_f );

    if( pvm_video_init() )
    {
        printf("Video init failed\n");
        exit(22);
    }

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
    //videotest();
    videotest_pbm();
    //videotest_overlay();
    getchar();
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

//pvm_json_test();exit(33);

    pvm_root_init();

#if 0
    test_json();
    //return 0;
#endif

    // Enable multithreading in user mode.
    // Does not work yet.
#if 0
    activate_all_threads();
#endif

    // TODO use stray catcher in pvm_test too
    //stray();

#if 0
//ui_loop( argc, argv, "test");
    printf("\nPhantom code finished\n" );
    //getchar();
    //{ char c; read( 0, &c, 1 ); }
    sleep(100);
#else
    printf("\nPhantom code finished\n" );

    if(arg_run_debugger)
    {
        dbg_init();
        kernel_debugger();
    }
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
           "Usage: pvm_test [-flags] [env_name=env_val]\n\n"
           "Flags:\n"
           "\t-di\t- debug (print) instructions\n"
           "\t-dt\t- print trace (class, method, IP)\n"
           "\t-dd\t- on finish start kernel debugger\n"
           "\t-l<fn>\t- set kernel log file name\n"
           "\t-o<fn>\t- set kernel console output file name\n"
           "\t-h\t- print this\n"
           "\n"
           "Env:\n"
           "\troot.shell=.ru.dz.phantom.system.shell\n"
           "\troot.init=.ru.dz.phantom.system.init\n"
           "\troot.boot=.ru.dz.phantom.system.boot\n"

           );
}

//extern int debug_print_instr;

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
        /*
        case 'c':
            {
                arg++; // skip 'c'
                const char *start_class = arg;
                printf("Will start ");
            }
            break;
        */

        case 'd':
            {
                char c;
                arg++;
                while( (c = *arg++ ) != 0 )
                {
                    switch(c) {
                    case 'i': debug_print_instr = 1; break;
                    case 'd': arg_run_debugger = 1; break;
                    case 't': debug_trace = 1; break;

                    default: usage(); exit(22);
                    }
                }
            }
            break;

        case 'l':
            {
                const char *fn = arg+1;
                printf("Will log to '%s'\n", fn );
                win_hal_open_kernel_log_file( fn );
            }
            break;

        case 'o':
            {
                const char *fn = arg+1;
                printf("Will send console output to '%s'\n", fn );
                win_hal_open_kernel_out_file( fn );
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



