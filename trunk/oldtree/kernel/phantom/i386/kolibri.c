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
#include <thread_private.h>

#include <kernel/trap.h>
#include <ia32/proc_reg.h>

#include <kernel/libkern.h>

#include <unix/uuprocess.h>
#include <unix/uufile.h>
#include <kunix.h>

#include <kernel/unix.h>
#include <time.h>
#include <fcntl.h>

#include <compat/kolibri.h>
#include <video/color.h>
#include <video.h>

#include "../svn_version.h"

#define fs_u_ptr( ___up, ___sz  ) ({ \
    addr_t a = (addr_t)(___up); \
    a += (addr_t)u->mem_start;  \
    if(a+((size_t)___sz) > (addr_t)u->mem_end) { return KERR_FAULT; } \
    if((___sz) == 0 && check_u_string( (const char *)a, (addr_t)u->mem_end ) ) { return KERR_FAULT; }; \
    (void *)a; \
    })


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

static struct kolibri_color_defaults color_defaults =
{
    { 0, 0xFF, 0, 0xFF },       // border
    { 0, 0xFF, 0xFF, 0xFF },    // header

    { 40, 40, 40, 0xFF },    // button_color;
    { 0xFF, 0xFF, 0xFF, 0xFF },    // button_text_color
    { 0xE0, 0xE0, 0xE0, 0xFF },    // title_text_color;

    { 10, 10, 10, 0xFF },    // work_color;
    { 40, 40, 40, 0xFF },    // work_button_color;
    { 0xFF, 0xFF, 0xFF, 0xFF },    // work_button_text_color;

    { 0xFF, 0xFF, 0xFF, 0xFF },    // work_text_color;
    { 0, 0, 0xFF, 0xFF },    // work_graph_color;

};

/*
static void *u_ptr( addr_t ___up, size_t sz )
{
    addr_t a = (addr_t)___up; a += (addr_t)u->mem_start; if(a > (addr_t)u->mem_end) return;

    (void *)a;
}
*/


// TODO per thread!!
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

    ks->key_input_scancodes = 0;

    return ks;
}

//  ------------------------------------------------  !!!! TODO who calls me?
void destroy_kolibri_state(uuprocess_t *u)
{
    struct kolibri_process_state *ks = u->kolibri_state;

    SHOW_FLOW0( 1, "Destroy Kolibri state" );

    if( ks == 0 )
        return;

    u->kolibri_state = 0;

    hal_sem_destroy( &ks->event );
    hal_mutex_destroy( &ks->lock );

    destroy_pool(ks->buttons);

    free(ks);
}


// ------------------------------------------------
// Internal kernel funcs
// ------------------------------------------------
static void kolibri_sys_internal( uuprocess_t *u, struct kolibri_process_state * ks, struct trap_state *st )
{
    (void) u;
    (void) ks;

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

// ------------------------------------------------
// info/ctl funcs
// ------------------------------------------------


static void kolibri_sys_info( uuprocess_t *u, struct kolibri_process_state * ks, struct trap_state *st )
{
    (void) u;
    (void) ks;

    //int dummy = 0;

    switch(st->ebx)
    {
    case 2:
        {
            int slot = st->ecx;
            SHOW_ERROR( 0, "Unimplemented Kolibri kill by slot %d", slot );
        }
        break;

    case 5: // TODO FAKE - get cpu freq
        st->eax = 1 << 30; // about 1ghz ?
        break;

    case 8: // spkr
        if( st->ecx == 1 ) // speaker?
            st->eax = 0; // speaker is off
        break;

    case 9: // halt/reboot
        // ecx 2 = turn off, 3 - reboot
        // st->eax = 0; // ok
        break;

    case 13: // get version
        {
            static struct kolibri_kernel_version vs = { 0, 7, 7, 0, 0, 0 };

            static char phan[] = "PHAN";
            static char tom[] = "TOM ";

            if(vs.svn_rev == 0)
            {
                sscanf( svn_version(), "%d", &vs.svn_rev );
                SHOW_FLOW( 1, "Report SVN ver %d to Kolibri", vs.svn_rev );
            }

            int movsz = umin( sizeof(struct kolibri_kernel_version), 16 );
            void *dest = u_ptr(st->ecx, 16);
            memmove( dest, &vs, movsz );

            if( st->esi == *(u_int32_t *)&phan[0] && st->edi == *(u_int32_t *)&tom[0])
            {
                st->esi = PHANTOM_VERSION_MAJ;
                st->edi = PHANTOM_VERSION_MIN;
                st->eax = *(u_int32_t *)&phan[0];
                st->edx = *(u_int32_t *)&tom[0];
            }
        }
        break;

    default:
        SHOW_ERROR( 0, "Unimplemented Kolibri 18 syscall ebx = %d", st->ebx );
        st->eax = 2; // Most funcs return nonzero retcode in eax.
    }
}


// ------------------------------------------------
// file funcs
// ------------------------------------------------

#define ret_on_err(_e) switch(e) {\
    case ENOTDIR: \
    case EEXIST: \
    case ENOENT:	return KERR_NOENT; \
    case EPERM: \
    case EACCES:	return KERR_ACCESS_DENIED; \
    case EIO:           return KERR_IO; \
    case ENOMEM:        return KERR_NOMEM; \
    case EFAULT:        return KERR_FAULT; \
    case EROFS: \
    case ENODEV:        return KERR_IVALID_FUNC; \
    \
    default: 		return KERR_IVALID_FUNC; \
    }



static int kolibri_get_fn( char *ofn, size_t ofnlen, kolibri_FILEIO *fi, uuprocess_t *u )
{
    char *ifn;

    if( fi->r2 )
    {
        ifn = fs_u_ptr( &(fi->r2), 0 );
    }
    else
    {
        ifn = fs_u_ptr( fi->name, 0 );
    }
    if( strlen( ifn ) >= ofnlen )
        return KERR_NOENT;

    if( 0 == strnicmp( ifn, "/sys/", 5 ) )
    {
        // Replace /sys with /amnt0 - first disk automount dir
        snprintf( ofn, ofnlen, "/amnt0/%s", ifn+5 );
    }
    else
        strlcpy( ofn, ifn, ofnlen );

    SHOW_FLOW( 5, "fn '%s'", ofn );

    return 0;
}

static int kolibri_sys_file( uuprocess_t *u, struct kolibri_process_state * ks, struct trap_state *st )
{
    (void) ks;

    kolibri_FILEIO *fi = fs_u_ptr( st->ebx, sizeof(kolibri_FILEIO) );
    char fn[FS_MAX_PATH_LEN];

    int rc = kolibri_get_fn( fn, sizeof(fn), fi, u );
    if( rc )
        return rc;

    void *data = 0;

    int fd, nbyte;
    int mode = 0;
    errno_t e = 0;

    switch(fi->cmd)
    {
    case 0: // Read file
    case 1: // Read dir
    case 2: // Create file
    case 3: // Write file
        data = fs_u_ptr( fi->buff, fi->count );
    }

    switch(fi->cmd)
    {
    case 0: // Read file
        {
            SHOW_FLOW( 5, "read %d @ %d", fi->count, fi->offset );
            e = k_open( &fd, fn, O_RDONLY, 0 );
            ret_on_err(e);
            e = k_seek( 0, fd, fi->offset, 0 /* seek set */ );
            if( e )
            {
                k_close( fd );
                ret_on_err(e);
            }
            e = k_read( &nbyte, fd, data, fi->count );
            SHOW_FLOW( 5, "read %d done", nbyte );
            k_close( fd );
            st->ebx = nbyte;
            ret_on_err(e);
            break;
        }

        {
        case 2: // Create file
            mode |= O_CREAT;
        case 3: // Write file
            mode |= O_RDWR;
            e = k_open( &fd, fn, mode, 0666 );
            ret_on_err(e);
            e = k_write( &nbyte, fd, data, fi->count );
            k_close( fd );
            st->ebx = nbyte;
            ret_on_err(e);
            break;
        }

    case 4: // Set file size
        rc = usys_truncate( &e, u, fn, fi->offset );
        break;

    case 7: // Run program
        rc = usys_run( &e, u, fn, 0, 0, 0 );
        break;

    case 8: // Remove file
        rc = usys_rm( &e, u, fn );
        break;

    case 9: // Create dir
        rc = usys_mkdir( &e, u, fn );
        break;

    case 1: // Read dir
    case 5: // Get file info
    case 6: // Set file attr
    default:
        SHOW_ERROR( 0, "Unsupported fileio cmd %d", fi->cmd );
        return KERR_ACCESS_DENIED;
    }

    ret_on_err(e);
    return 0;
}



// ------------------------------------------------
// Graphical calls helpers
// ------------------------------------------------

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


// ------------------------------------------------
// Main Kolibro syscall dispatcher
// ------------------------------------------------


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

            // negative 16bit num
            if( xpos & 0x8000 )
                xpos = get_screen_xsize() - (0xFFFF - (0xFFFF & xpos));

            if( ypos & 0x8000 )
                ypos = get_screen_ysize() - (0xFFFF - (0xFFFF & ypos));

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

    case 2: // read key
        {
            st->eax = 1; // no key
            //st->eax = 0x0000FF00 & (ch << 8); // have key
        }
        break;

    case 3: // get time
        {
            struct tm *tmp;
            struct tm mytime;
            tmp = current_time;
            mytime = *tmp;

            u_int32_t bcd_s = BCD_BYTE(mytime.tm_sec);
            u_int32_t bcd_m = BCD_BYTE(mytime.tm_min);
            u_int32_t bcd_h = BCD_BYTE(mytime.tm_hour);

            st->eax = (bcd_s << 16) | (bcd_m << 8) | bcd_h;
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

    case 5: // sleep
        {
            int msec = st->ebx * 10;
            hal_sleep_msec(msec);
        }
        break;

    case 7: // draw image
        {
            rect_t r;

            r.x = st->edx >> 16;
            r.y = st->edx & 0xFFFF;

            r.xsize = st->ecx >> 16;
            r.ysize = st->ecx & 0xFFFF;

            r.y = ks->win->ysize - r.y;

            int npixels = r.xsize * r.ysize;

            // st->ebx - BGR bitmap ptr
            const struct rgb_t *src = u_ptr( st->ebx, npixels*3 );

            rgba_t *pixels = calloc( sizeof(rgba_t), npixels );
            rgb2rgba_move( pixels, src, npixels );

            bitmap2bitmap(
                          ks->win->pixel, ks->win->xsize, ks->win->ysize, r.x, r.y,
                          pixels, r.xsize, r.ysize, 0, 0, r.xsize, r.ysize );
            free( pixels );

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

            // st->ebx - BGR bitmap ptr
            const struct rgb_t *src = u_ptr( st->ebx, npixels*3 );

            cb->pixels = calloc( sizeof(rgba_t), npixels );
            rgb2rgba_move( cb->pixels, src, npixels );

            bitmap2bitmap(
                          ks->win->pixel, ks->win->xsize, ks->win->ysize, r.x, r.y,
                          cb->pixels, r.xsize, r.ysize, 0, 0, r.xsize, r.ysize );

            // TODO paint button bitmap
            // TODO process button

            pool_release_el( ks->buttons, bh );
        }
        break;

    case 9: // Get thread info
        {
            st->eax = MAX_THREADS;

            struct kolibri_thread_info *ti = u_ptr( st->ebx, 1024 );
            int slot = st->ecx;

            if( slot >= MAX_THREADS )
            {
            no_slot:
                ti->state = 9; // Slot is empty
                break;
            }

            tid_t qtid;

            if( slot == -1 )
                qtid = get_current_tid();
            else
            {
                if(0 == phantom_kernel_threads[slot])
                    goto no_slot;

                qtid = phantom_kernel_threads[slot]->tid;
            }

            phantom_thread_t *ptinfo = get_thread(qtid);
            if(0 == ptinfo)
                goto no_slot;

            phantom_thread_t tinfo = *ptinfo;

            bzero( ti, sizeof(struct kolibri_thread_info) );

            strncpy( ti->name, tinfo.name, 11 );
            ti->tid = qtid;

            if( tinfo.sleep_flags & THREAD_SLEEP_ZOMBIE )
                ti->state = 3; // dies
            else if( tinfo.sleep_flags )
                ti->state = 1; // blocked
            else
                ti->state = 0; // run


            // Fake
            ti->cpu_usage = 0;
            ti->win_z_order = 0;
            ti->ecx_win_slot = 1; // Oh no...
            ti->mem_addr = 0;
            ti->mem_size = 4096; // dunno how to access - via pid?
            ti->event_mask = 0x7; // good guess :)
        }
        break;

    case 11: // get event bits
        st->eax = ks->event_state & ks->event_mask;
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
            {
                SHOW_FLOW0( 1, "Repaint" );
                drv_video_window_update( ks->win );
            }
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

    case 14: // get screen size
        {
            u_int32_t xs = 0xFFFF & (get_screen_xsize() - 1);
            u_int32_t ys = 0xFFFF & (get_screen_ysize() - 1);
            st->eax = (xs << 16) | ys;
        }
        break;

    case 18: // Info + control
        kolibri_sys_info(u,ks,st);
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

            //SHOW_FLOW( 0, "pnum '%s'", p );

            // TODO clear bg
            drv_video_font_draw_string( ks->win, font, p, fg, xpos, ypos );

        }
        break;

    case 48: // colors & styles
        {
            switch(st->ebx)
            {
            case 3: // Get defaults
                {
                    size_t sz = st->edx;
                    if( sz > sizeof(color_defaults) ) sz =- sizeof(color_defaults);
                    void *buf = u_ptr( st->ecx, sz );
                    memcpy( buf, &color_defaults, sz );
                }
                break;

            default:
                SHOW_ERROR( 0, "Unimplemented 48 syscall ebx = %d ecx = %d", st->ebx, st->ecx );
                break;
            }
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

    case 66: // keybd
        {
            switch( st->ebx )
            {
            case 1:
                ks->key_input_scancodes = st->ecx;
                break;

            case 2: // get inp mode
                st->eax = ks->key_input_scancodes;
                break;

            case 3: // get shifts state
                st->eax = 0; // TODO - stub - nothing
                break;

            case 4: // set hot key
                st->eax = 0; // TODO - stub - does nothing
                break;

            default:
                SHOW_ERROR( 0, "Unimplemented 66 syscall ebx = %d", st->ebx );
                break;


            }
        }
        break;

    case 68:
        // Internal sys funcs
        kolibri_sys_internal(u,ks,st);
        break;

    case 70:
        // File funcs
        st->eax = kolibri_sys_file(u,ks,st);
        break;

    default:
        SHOW_ERROR( 0, "Unimplemented Kolibri syscall eax = %d", st->eax );
        st->eax = 2; // Most funcs return nonzero retcode in eax.
    }
}












































