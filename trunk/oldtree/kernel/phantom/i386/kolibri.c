/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Kolibri syscalls support
 *
**/

#define DEBUG_MSG_PREFIX "Kolibri"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_types.h>
#include <phantom_libc.h>
#include <threads.h>

#include <kernel/trap.h>
#include <ia32/proc_reg.h>
#include <unix/uuprocess.h>
#include <compat/kolibri.h>
#include <video/color.h>

#define u_ptr( ___up ) ({ addr_t a = (addr_t)___up; a += (addr_t)u->mem_start; if(a > (addr_t)u->mem_end) return; (void *)a; })



static struct kolibri_process_state * get_kolibri_state(uuprocess_t *u)
{
    if( u->kolibri_state == 0 )
        u->kolibri_state = calloc( 1, sizeof(struct kolibri_process_state) );
    else
        return u->kolibri_state;

    struct kolibri_process_state *ks = u->kolibri_state;
    assert(ks);

    hal_sem_init( &ks->event, "KolibriEv" );

    return ks;
}


static void kolibri_sys_internal( uuprocess_t *u, struct trap_state *st )
{
    (void) u;

    int dummy = 0;

    switch(st->ebx)
    {
    case 0:
        st->eax = dummy++; // TODO n of context switches
        break;

    case 1: // yield
        hal_sleep_msec(0);
        break;

    case 2: // rdpmc junk
        if( st->ecx == 0 )
            st->eax = get_cr4();
        else
            st->eax = 0x60000000; // cache is on
        break;

    case 11: // init heap
        st->eax = 0; // fail
        break;

    case 12: // calloc
        {
            size_t size = st->ecx;
            SHOW_FLOW( 2, "malloc %d", size );
            st->eax = 0; // fail
        }
        break;

    case 13: // free
        {
            addr_t mem = st->ecx;
            SHOW_FLOW( 2, "free @ 0x%p", mem );
            st->eax = 0; // fail
        }
        break;

    default:
        SHOW_ERROR( 0, "Unimplemented Kolibri 68 syscall ebx = %d", st->ebx );
        st->eax = 2; // Most funcs return nonzero retcode in eax.
    }
}


#define GET_POS_SIZE \
    int xpos = st->ebx >> 16;      \
    int ypos = st->ecx >> 16;      \
                                   \
    int xsize = st->ebx & 0xFFFF;  \
    int ysize = st->ecx & 0xFFFF;


static inline color_t i2color( u_int32_t ic )
{
    color_t fill;

    fill.r = (ic >> 16) & 0xFF;
    fill.g = (ic >> 8) & 0xFF;
    fill.b = ic & 0xFF;

    fill.a = 0xFF;

    return fill;
}


void kolibri_sys_dispatcher( struct trap_state *st )
{
    tid_t tid = get_current_tid();
    pid_t pid;
    assert( !t_get_pid( tid, &pid ));
    uuprocess_t *u = proc_by_pid(pid);

    struct kolibri_process_state * ks = get_kolibri_state(u);

    switch(st->eax)
    {
    case -1:
        SHOW_FLOW( 2, "exit %d", 0 );
        hal_exit_kernel_thread();
        panic("no exit?");
        break;


    case 0: // make app win
        {
            GET_POS_SIZE

            color_t fill = i2color(st->edx);

            SHOW_FLOW( 2, "make app win %dx%d @ %d/%d", xsize, ysize, xpos, ypos );

            // TOOD clip
            if( xsize > 2000 ) xsize = 2000;
            if( ysize > 2000 ) ysize = 2000;

            if( ks->win )
                break;

            // TODO edx/edi/esi - colors, flags

            ks->win = drv_video_window_create(
                                              xsize, ysize,
                                              xpos, ypos, fill, "Kolibri" );

            drv_video_window_update( ks->win );

        }
        break;

    case 4: // draw text line
        {
            if( !ks->win )
                break;

            int xpos = st->ebx >> 16;
            int ypos = st->ebx & 0xFFFF;
            color_t textc = i2color(st->ecx);
            //color_t bgc = i2color(st->edi);
            int len = st->esi;

            char *str = u_ptr(st->edx);

            int nfont = (st->ecx >> 28) & 0x3;
            int asciiz = (st->ecx >> 30) & 0x1;
            //int clear_bg = (st->ecx >> 29) & 0x1;

            //SHOW_FLOW( 2, "string bg", xsize, ysize, xpos, ypos );

            if( (!asciiz) && len > 255 )
                break;

            char buf[256+1];

            if( !asciiz )
            {
                strlcpy( buf, str, len );
                str = buf;
            }

            const drv_video_font_t *font = nfont ? &drv_video_8x16ant_font : &drv_video_8x16cou_font ;

            drv_video_font_draw_string( ks->win, font, str, textc, xpos, ypos );

        }
        break;

    case 12: // begin/and repaint app win
        {
            if( !ks->win )
                break;

            if( st->ebx == 1 )
            {
                // TODO remove all buttons
            }

            if( st->ebx == 2 )
                drv_video_window_update( ks->win );
        }
        break;

    case 13: // draw rect
        {
            if( !ks->win )
                break;

            GET_POS_SIZE;

            color_t c = i2color( st->edx );

            drv_video_window_draw_box( ks->win, xpos, ypos, xsize, ysize, c );
        }
        break;

    case 23: // wait 4 event with timeout
        {
            int timeout_msec = st->ebx * 10;
            if(timeout_msec == 0)
            {
                st->eax = 0;
                break;
            }

            int rc = hal_sem_acquire_etc( &ks->event, 1, SEM_FLAG_TIMEOUT, 1000*timeout_msec );
            if(rc)
            {
                st->eax = 0;
                break;
            }

            st->eax = 0; // TODO put event here
        }
        break;

    case 68:
        // Internal sys funcs
        kolibri_sys_internal(u,st);
        return;

    default:
        SHOW_ERROR( 0, "Unimplemented Kolibri syscall eax = %d", st->eax );
        st->eax = 2; // Most funcs return nonzero retcode in eax.
    }
}
