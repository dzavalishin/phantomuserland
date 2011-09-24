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
#include <kernel/unix.h>

#include <compat/kolibri.h>
#include <video/color.h>
#include <video.h>

//static void kolibri_sys_printn(uuprocess_t *u, struct trap_state *st );


#define u_ptr( ___up, ___sz  ) ({ \
    addr_t a = (addr_t)(___up); \
    a += (addr_t)u->mem_start;  \
    if(a+((size_t)___sz) > (addr_t)u->mem_end) return; \
    if((___sz) == 0 && check_u_string( (const char *)a, (addr_t)u->mem_end ) ) return; \
    (void *)a; \
    })


static errno_t check_u_string( const char *a, addr_t mem_end )
{
    while( 1 )
    {
        if( ((addr_t)a) >= mem_end )
            return EINVAL;
        if( *a == 0 )
            return 0;
        a++;
    }
}

/*
static void *u_ptr( addr_t ___up, size_t sz )
{
    addr_t a = (addr_t)___up; a += (addr_t)u->mem_start; if(a > (addr_t)u->mem_end) return;

    (void *)a;
}
*/

static struct kolibri_process_state * get_kolibri_state(uuprocess_t *u)
{
    if( u->kolibri_state == 0 )
        u->kolibri_state = calloc( 1, sizeof(struct kolibri_process_state) );
    else
        return u->kolibri_state;

    struct kolibri_process_state *ks = u->kolibri_state;
    assert(ks);

    hal_sem_init( &ks->event, "KolibriEv" );
    hal_mutex_init( &ks->lock, "KolibriPr" );

    ks->buttons = create_pool();
    ks->buttons->flag_autoclean = 1;
    ks->buttons->flag_autodestroy = 0;

    ks->event_mask = 0x7;

    return ks;
}

//  ------------------------------------------------  !!!! TODO who calls me?
void destroy_kolibri_state(uuprocess_t *u)
{
    struct kolibri_process_state *ks = u->kolibri_state;

    if( ks == 0 )
        return;

    u->kolibri_state = 0;

    hal_sem_destroy( &ks->event );
    hal_mutex_destroy( &ks->lock );

    destroy_pool(ks->buttons);

    free(ks);
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


#define GET_POS_SIZE() \
    int xpos = st->ebx >> 16;      \
    int ypos = st->ecx >> 16;      \
                                   \
    int xsize = st->ebx & 0xFFFF;  \
    int ysize = st->ecx & 0xFFFF;


#define GET_POS_RECT(___r) ({\
    (___r).x = st->ebx >> 16;      \
    (___r).y = st->ecx >> 16;      \
                                   \
    (___r).xsize = st->ebx & 0xFFFF;  \
    (___r).ysize = st->ecx & 0xFFFF;  \
    })



static inline color_t i2color( u_int32_t ic )
{
    color_t fill;

    fill.r = (ic >> 16) & 0xFF;
    fill.g = (ic >> 8) & 0xFF;
    fill.b = ic & 0xFF;

    fill.a = 0xFF;

    return fill;
}



static errno_t kolibri_kill_button(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    struct kolibri_button *cb = el;
    int kill_id = (int)arg;

    if( cb->id == kill_id )
    {
        pool_destroy_el( pool, handle );
    }
    return 0;
}

void kolibri_sys_dispatcher( struct trap_state *st )
{
    tid_t tid = get_current_tid();
    pid_t pid;
    assert( !t_get_pid( tid, &pid ));
    uuprocess_t *u = proc_by_pid(pid);

    struct kolibri_process_state * ks = get_kolibri_state(u);

    // used when call unix syscall workers
    int ret = 0;
    errno_t err = 0;


    switch(st->eax)
    {
    case -1:
        SHOW_FLOW( 2, "exit %d", 0 );
        hal_exit_kernel_thread();
        panic("no exit?");
        break;


    case 0: // make app win
        {
            GET_POS_SIZE();

            ypos = get_screen_ysize() - ypos - ysize;

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

            ypos = ks->win->ysize - ypos;

            color_t textc = i2color(st->ecx);
            //color_t bgc = i2color(st->edi);
            int len = st->esi;

            char *str = u_ptr(st->edx, 0);

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

            // TODO clear bg
            drv_video_font_draw_string( ks->win, font, str, textc, xpos, ypos );

        }
        break;

    case 8: // button
        {
            if( !ks->win )
                break;

            u_int32_t id = st->edx;

            if( id & 0x80000000 )
            {
                id &= 0xFFFFFF;
                // remove button

                pool_foreach( ks->buttons, kolibri_kill_button, (void *)id );
                break;
            }
            // make button

            int nopaint = id & (1<<30);
            int noborder = id & (1<<29);

            id &= 0xFFFFFF;

            rect_t r;
            GET_POS_RECT(r);

            r.y = ks->win->ysize - r.y;

            int npixels = r.xsize * r.ysize;
            // st->ebx - BGR bitmap ptr

            color_t c = i2color( st->esi );

            drv_video_window_fill_rect( ks->win, c, r );

            pool_handle_t bh = pool_create_el( ks->buttons, calloc( 1, sizeof(struct kolibri_button) ) );
            struct kolibri_button *cb = pool_get_el( ks->buttons, bh );

            if( !cb )
            {
                // SHOW_ERROR
                break;
            }

            cb->id = id;
            cb->r = r;
            cb->npixels = npixels;
            cb->flag_nopaint = nopaint;
            cb->flag_noborder = noborder;

            // TODO paint button bitmap
            // TODO process button

            pool_release_el( ks->buttons, bh );
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

            GET_POS_SIZE();
            ypos = ks->win->ysize - ypos;

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

    case 30: // chdir/getcwd
        switch( st->ebx )
        {
        case 1: // chdir
            {
                const char *path = u_ptr( st->ecx, 0 );
                ret = usys_chdir( &err, u, path );
            }
            break;
        case 2: // getcwd
            {
                char *buf = u_ptr( st->ecx, st->edx );
                ret = usys_getcwd( &err, u, buf, st->edx );
                if( ret )
                    st->eax = 0;
                else
                    st->eax = strlen(buf);
            }
            break;
        }
        break;

    case 38: // draw line
        {
            if( !ks->win )
                break;

            int xpos = st->ebx >> 16;
            int ypos = st->ecx >> 16;

            int xend = st->ebx & 0xFFFF;
            int yend = st->ecx & 0xFFFF;

            ypos = ks->win->ysize - ypos;
            yend = ks->win->ysize - yend;

            // TODO inverse draw
            color_t c = i2color( st->edx );

            drv_video_window_draw_line( ks->win, xpos, ypos, xend, yend, c );
        }
        break;

    case 40: // set event mask
        ks->event_mask = st->ebx;
        break;

    case 47: // print number
        if( !ks->win )
            break;
        {
            color_t fg = i2color( st->esi );
            color_t bg = i2color( st->edi );

            int fill_bg = st->esi & (1<< 30);

            bg.a = fill_bg ? 0xFF : 0;

            int xpos = st->edx >> 16;
            int ypos = st->edx & 0xFFFF;

            ypos = ks->win->ysize - ypos;

            int is_ptr = st->ebx & 0xF;
            int is_qword = (st->ebx >> 30) & 1;
            int no_lead_zeros = (st->ebx >> 31) & 1;

            if(is_qword) is_ptr = 1;

            int radix = 10;

            switch( st->ebx & 0xF0 )
            {
            case 0: radix = 10; break;
            case 1: radix = 16; break;
            case 2: radix = 2; break;
            }

            int ndigits = (st->ebx >> 16) & 0x3F;

            uintmax_t num = st->ecx;

            if( is_ptr )
            {
                void *buf = u_ptr( st->ecx, is_qword ? 8 : 4 );

                if(is_qword)
                    num = *(u_int64_t *)buf;
                else
                    num = *(u_int32_t *)buf;
            }

            char buf[64];

            int cnt = 0;
            int limit = sizeof(buf)-2;
            char *p = buf+sizeof(buf)-1;

            *--p = 0;
            do {
                *--p = hex2ascii(num % radix);
                cnt++;
                if( --limit <= 0 )
                {
                    *p = '!';
                    break;
                }
            } while (num /= radix);

            while( (cnt < ndigits) && (limit > 0) )
            {

                *--p = no_lead_zeros ? ' ' : '0';
                cnt++;
                limit--;
            }

            //const drv_video_font_t *font = nfont ? &drv_video_8x16ant_font : &drv_video_8x16cou_font ;
            const drv_video_font_t *font = &drv_video_8x16cou_font ;

            SHOW_FLOW( 0, "pnum '%s'", p );

            // TODO clear bg
            drv_video_font_draw_string( ks->win, font, p, fg, xpos, ypos );

        }
        break;


    case 54: // ?
        st->eax = 0x12345678;
        break;

    case 63: // debug board
        {
#define DBB_SIZE 4096
            static char dbb[DBB_SIZE];
            static char *pp = dbb;
            static char *gp = dbb;

            hal_mutex_lock( &ks->lock );

            if( pp >= dbb + DBB_SIZE )
                pp = dbb;

            if( gp >= dbb + DBB_SIZE )
                gp = dbb;

            switch( st->ebx )
            {

            case 1: // wr
                *pp++ = st->ecx;
                break;

            case 2: // rd
                if( pp == gp )
                {
                    st->eax = 0;
                    st->ebx = 0;
                }
                else
                {
                    st->eax = *gp++;
                    st->ebx = 1;
                }
            break;
            }

            hal_mutex_unlock( &ks->lock );

        }
        break;

    case 68:
        // Internal sys funcs
        kolibri_sys_internal(u,st);
        break;

    default:
        SHOW_ERROR( 0, "Unimplemented Kolibri syscall eax = %d", st->eax );
        st->eax = 2; // Most funcs return nonzero retcode in eax.
    }
}












































