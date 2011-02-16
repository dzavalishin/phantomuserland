/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Stray pointer catch logic.
 *
 *
**/


#define DEBUG_MSG_PREFIX "stray"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include "misc.h"
#include <stdio.h>
#include <hal.h>
#include <threads.h>

#ifdef STRAY_CATCH_SIZE

// num of src files
#define MAXCR 1024

struct checkrec
{
        void * buf;
        int    size;

        const char * name;
};

static struct checkrec  cr[MAXCR];
static int ncr = 0;

void phantom_debug_register_stray_catch( void *buf, int bufs, const char *name )
{
    cr[ncr].buf = buf;
    cr[ncr].size = bufs;
    cr[ncr].name = name;
    ncr++;
}

void stray(void)
{
    int i;
    for(i = 0; i < MAXCR; i++ )
    {
        if( cr[i].size == 0 )
            break;

        char *p = cr[i].buf;
        char *e = p + cr[i].size;

        if( *p == '0' )
        {
            if( strncmp( p, "0000", 4 ) )
                goto fail;
        }

        p += 4;

        for( ; p < e; p++ )
        {
            if( *p )
            {
            fail:
                printf("!!!!!! stray data at %p : %s !!!!!!\n", p, cr[i].name );

                int len = e-p;
                if( len > 16 ) len = 16;

                hexdump( p, len, 0, 0 );

                return;
            }
        }
    }
}

static void stray_catch_thread(void)
{
    hal_set_thread_name("StrayCheck");
    printf("Stray checker running, %d windows\n", ncr);

    while(1)
    {
        // 1 sec
        //hal_sleep_msec(1000);
        hal_sleep_msec(50);

        stray();
    }
}


#endif // STRAY_CATCH_SIZE


void init_stray_checker(void)
{
#ifdef STRAY_CATCH_SIZE
    hal_start_kernel_thread(&stray_catch_thread);

#if 0
    int i;
    for(i = 0; i < MAXCR; i++ )
    {
        if( cr[i].size == 0 )
            break;

        printf("%03d: %8p (%6d) %s\n", i, cr[i].buf, cr[i].size, cr[i].name );
    }
#endif

#endif // STRAY_CATCH_SIZE

}



