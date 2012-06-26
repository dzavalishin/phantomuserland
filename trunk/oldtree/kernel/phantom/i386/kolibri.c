/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Kolibri syscalls support
 *
**/

#if HAVE_KOLIBRI

#define DEBUG_MSG_PREFIX "Kolibri"
#include <debug_ext.h>
#define debug_level_flow 8
#define debug_level_error 9
#define debug_level_info 10

#include <phantom_types.h>
#include <phantom_libc.h>
#include <threads.h>
#include <thread_private.h>

#include <kernel/trap.h>
#include <kernel/init.h>
#include <kernel/libkern.h>
#include <kernel/profile.h>

#include <ia32/proc_reg.h>


#include <kernel/unix.h>
#include <unix/uuprocess.h>
#include <unix/uufile.h>
#include <kunix.h>

#include <time.h>
#include <fcntl.h>

#include <compat/kolibri.h>
#include <video/color.h>
#include <video/screen.h>
#include <video/window.h>
#include <video/font.h>
#include <video/vops.h>

#include "../svn_version.h"

#define REPAINT_TIMEOUT_MS 5


static void kolibri_reload_window_alpha(struct kolibri_process_state *ks);

static void request_update_timer( struct kolibri_process_state *ks, int msec );
static void cancel_update_timer( struct kolibri_process_state *ks );

static tid_t kolibri_start_thread( uuprocess_t *u, struct kolibri_process_state *ks, addr_t eip, addr_t esp );

static void kolibri_sys_hw( uuprocess_t *u, struct kolibri_process_state * ks, struct trap_state *st );

static void kolibri_send_event( tid_t tid, u_int32_t event_id );
static u_int32_t get_event_bits(uuprocess_t *u, struct kolibri_process_state *ks);
static u_int32_t eat_event_bit(uuprocess_t *u, struct kolibri_process_state *ks);

static int kolibri_copy_fn( char *ofn, size_t ofnlen, const char *ifn );



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
    ks->buttons->flag_autodestroy = 1;

    ks->event_mask = 0x7;
    ks->event_bits = 1; // on first request return 'redraw' event - some code depends on it
    hal_sem_release( &ks->event ); // make sure app will receive that event

    ks->key_input_scancodes = 0;

    ks->win_alpha_scale = 0;
    ks->win_user_alpha = 0;

    ks->keys = wtty_init();
    assert(ks->keys);

    //ks->win_update_timer
    request_update_timer( ks, 1000 ); // to make sure we can call cancel... on destroying kolibri state

    return ks;
}

//  ------------------------------------------------  !!!! TODO who calls me?
void destroy_kolibri_state(uuprocess_t *u)
{
    struct kolibri_process_state *ks = u->kolibri_state;

    SHOW_FLOW0( 1, "Destroy Kolibri state" );

    if( ks == 0 )
        return;

    if( ks->win )
    {
        drv_video_window_free(ks->win);
        ks->win = 0;
    }


    u->kolibri_state = 0;

    cancel_update_timer( ks );

    hal_sem_destroy( &ks->event );
    hal_mutex_destroy( &ks->lock );

    destroy_pool(ks->buttons);

    free(ks);
}

static int upd_timer_set = 0;

static void win_timed_update( void *arg )
{
    struct kolibri_process_state *ks = arg;
    if(!ks->win)
        return;
    upd_timer_set = 0;

    if(!ks->win_update_prevent)
        w_update( ks->win );
}

static void request_update_timer( struct kolibri_process_state *ks, int msec )
{
    if(upd_timer_set) return;
    upd_timer_set = 1;
    set_net_timer( &ks->win_update_timer, msec, win_timed_update, ks, 0 );
}

static void cancel_update_timer( struct kolibri_process_state *ks )
{
    upd_timer_set = 0;
    cancel_net_timer( &ks->win_update_timer );
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

    case 3: // TODO read msr
        {
            int msr_no = st->edx;
            SHOW_FLOW( 2, "read MSR @ 0x%x", msr_no );
            st->eax = 0; // fail
            st->edx = 0; // top dword
        }
        break;

    case 11: // TODO init heap
        st->eax = 0; // fail
        break;

    case 12: // TODO calloc
        {
            size_t size = st->ecx;
            SHOW_FLOW( 2, "malloc %d", size );
            st->eax = 0; // fail
        }
        break;

    case 13: // TODO free
        {
            addr_t mem = st->ecx;
            SHOW_FLOW( 2, "free @ 0x%p", mem );
            st->eax = 0; // fail
        }
        break;

    case 16: // load driver
        {
            const char *uDriverName = u_ptr( st->ecx, 0 );
            SHOW_FLOW( 2, "load driver %s", uDriverName );
            st->eax = 0; // fail
        }
        break;

    case 19: // TODO load DLL
        {
            const char *uDllName = u_ptr( st->ecx, 0 );
            //addr_t mem = st->ecx;

            char fn[FS_MAX_PATH_LEN];

            int rc = kolibri_copy_fn( fn, sizeof(fn), uDllName );
            if( rc )
            {
                st->eax = 0; // fail
                break;
            }
            SHOW_FLOW( 2, "load DLL %s", fn );
            st->eax = 0; // fail
        }
        break;

    case 22: // open SHM
        {
            const char *uShmName = u_ptr( st->ecx, 0 );
            SHOW_FLOW( 2, "open shared memory '%s'", uShmName );
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

    //errno_t rc;

    //int dummy = 0;

    switch(st->ebx)
    {
    case 2:
        {
            int slot = st->ecx;
            SHOW_ERROR( 0, "Unimplemented Kolibri kill by slot %d", slot );
        }
        break;

    case 4: // get idle CPU timeslots
        st->eax = 100-percpu_cpu_load[0]; // in fact we return CPU 0 idle percentage
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
        switch( st->ecx )
        {
        case 2:
            phantom_shutdown(0);
            break;
        case 3:
            phantom_shutdown(SHUTDOWN_FLAG_REBOOT);
            break;
        }

        //st->eax = rc ? -1 : 0;
        st->eax = -1; // returned? error.

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

    case 16: // get free mem, Kb
        st->eax = pahantom_free_phys_mem_kb();
        break;

    case 17: // get all mem, Kb
        st->eax = pahantom_total_phys_mem_kb();
        break;

    case 21: // get slot by tid
        {
            tid_t tid = st->ecx;
            //phantom_thread_t *tp = get_thread(int tid);
            // TODO t_tid_is_valid()
            if( (tid < 0) || (tid >= MAX_THREADS) || (phantom_kernel_threads[tid] == 0))
            {
                st->eax = 0; // fail
                break;
            }
            st->eax = tid; // slot == tid in Phantom
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
    case 0:            return 0; \
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


static int kolibri_copy_fn( char *ofn, size_t ofnlen, const char *ifn )
{
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
#if 1
    return kolibri_copy_fn( ofn, ofnlen, ifn );
#else
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
#endif
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
            // TODO usys_...!
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
            // TODO usys_...!
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

    case 5: // Get file info
        {
            data = fs_u_ptr( fi->buff, 40 );
            kolibri_FILEINFO *fi = data;

            struct stat ustat;

            // TODO usys_stat!
            rc = k_stat( fn, &ustat, 0 );
            if( rc )
            {
                SHOW_ERROR( 0, "Can't stat %s", fn );
                ret_on_err(e);
                break;
            }

            bzero( fi, 40 );

            if( ustat.st_mode & S_IFDIR )
                fi->attr |= 0x10; // dir
            else if(!(ustat.st_mode & _S_IFREG))
                fi->attr |= 0x04; // system
            else
                fi->attr |= 0x0; // regular

            fi->size = ustat.st_size;

            fi->size_high = 0;
            fi->flags = 0;

            break;
        }
    case 1: // Read dir
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

#define COORD_TO_REG(__x,__y) (((__x) & 0xFFFF) << 16)|((__y) & 0xFFFF)

static inline color_t i2color( u_int32_t ic )
{
    color_t fill;

    fill.r = (ic >> 16) & 0xFF;
    fill.g = (ic >> 8) & 0xFF;
    fill.b = ic & 0xFF;

    fill.a = 0xFF;

    return fill;
}




struct checkb
{
    kolibri_state_t *ks;
    int x;
    int y;
    int mouseb;
};


static void paint_button(window_handle_t win, struct kolibri_button *cb)
{
    w_fill_rect( win, cb->color, cb->r );

    if(!cb->flag_nopaint)
    {
        bitmap2bitmap_yflip(
                            win->w_pixel, win->xsize, win->ysize, cb->r.x, cb->r.y,
                            cb->pixels, cb->r.xsize, cb->r.ysize, 0, 0, cb->r.xsize, cb->r.ysize );
    }

    if(!cb->flag_noborder)
    {
        if( cb->mouse_in_bits & 1 )
            w_draw_rect( win, COLOR_BLACK, cb->r );
    }
}

static void paint_changed_button(window_handle_t win, struct kolibri_button *cb)
{
    if( (cb->mouse_in_bits & 1) == ( (cb->mouse_in_bits>>1) & 1 ) )
        return;

    paint_button( win, cb );
}

static errno_t kolibri_kill_button(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    struct kolibri_button *cb = el;
    int kill_id = (int)arg;

    if( cb->id == kill_id )
    {
        pool_release_el( pool, handle );
        pool_destroy_el( pool, handle );
    }
    return 0;
}

static errno_t kolibri_kill_any_button(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) el;
    (void) arg;
    pool_release_el( pool, handle );
    pool_destroy_el( pool, handle );
    return 0;
}


static errno_t do_kolibri_paint_changed_button(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    struct kolibri_button *cb = el;
    struct checkb *env = arg;

    paint_changed_button( env->ks->win, cb);
    return 0;
}

static void kolibri_paint_changed_buttons(kolibri_state_t *ks)
{
    struct checkb env;

    //SHOW_FLOW( 1, "button check @ %d.%d buttons %x", x, y, mouseb );

    env.x = -1;
    env.y = -1;
    env.ks = ks;
    env.mouseb = 0;

    pool_foreach( ks->buttons, do_kolibri_paint_changed_button, &env );
}


static errno_t do_kolibri_check_button(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    (void) pool;
    (void) handle;

    struct kolibri_button *cb = el;
    struct checkb *env = arg;

    cb->mouse_in_bits <<= 1;

    if( point_in_rect( env->x, env->y, &cb->r ) )
    {
        SHOW_FLOW( 1, "button @ %d.%d in range id %x", env->x, env->y, cb->id );
        cb->mouse_in_bits |= 1;

        if(env->mouseb)
        {
            env->ks->pressed_button_id = cb->id;
            env->ks->pressed_button_mouseb = env->mouseb;
            kolibri_send_event( get_current_tid(), EV_BUTTON );
        }
    }

    return 0;
}

static void kolibri_check_button(int x, int y, kolibri_state_t *ks, int mouseb )
{
    struct checkb env;

    SHOW_FLOW( 1, "button check @ %d.%d buttons %x", x, y, mouseb );

    env.x = x;
    env.y = y;
    env.ks = ks;
    env.mouseb = mouseb;

    pool_foreach( ks->buttons, do_kolibri_check_button, &env );
}


// ------------------------------------------------
// Main Kolibry syscall dispatcher
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

    if( ((int)st->eax) == -1 )
    {
        SHOW_FLOW( 2, "exit %d", 0 );
        hal_exit_kernel_thread();
        panic("no exit?");
    }

    switch(st->eax & 0xFF)
    {
    case 0: // make app win
        {
            pool_foreach( ks->buttons, kolibri_kill_any_button, 0 );

            GET_POS_SIZE();

            // Kolibri quirk - window size is one pixel bigger
            xsize++;
            ysize++;

            // negative 16bit num
            if( xpos & 0x8000 )
                xpos = scr_get_xsize() - (0xFFFF - (0xFFFF & xpos));

            if( ypos & 0x8000 )
                ypos = scr_get_ysize() - (0xFFFF - (0xFFFF & ypos));

            ypos = scr_get_ysize() - ypos - ysize;

            color_t fill = i2color(st->edx);

            u_int32_t wstyle = st->edx >> 24;

            u_int32_t _x = st->edx >> 28;
            bool have_title 		= _x & 0x01;
            //bool ref_client_area 	= _x & 0x02; // All coords are relative to client area
            bool no_fill 		= _x & 0x04;
            bool gradient_fill 		= _x & 0x08;

            if( wstyle == 1 ) no_fill = 1; // right?

            SHOW_FLOW( 2, "make app win %dx%d @ %d/%d", xsize, ysize, xpos, ypos );

            if(gradient_fill)
                SHOW_ERROR0( 2, "unimpl gradient fill" );

            // TOOD clip
            if( xsize > 2000 ) xsize = 2000;
            if( ysize > 2000 ) ysize = 2000;

            if( !ks->win )
            {
                // TODO edx/edi/esi - colors, flags

                ks->win = drv_video_window_create(
                                                  xsize, ysize,
                                                  xpos, ypos, fill, "Kolibri", WFLAG_WIN_DECORATED );

                if(!ks->win)
                    break;

                ks->win->eventDeliverSema = &ks->event;

                ks->defaultEventProcess = ks->win->inKernelEventProcess;
                ks->win->inKernelEventProcess = 0;
            }


            if(!no_fill) w_fill( ks->win, fill );

            if(have_title && ((wstyle == 3) || (wstyle == 4)))
            {
                const char *title = u_ptr(st->edi,0);
                w_set_title( ks->win, title );
            }

            //w_update( ks->win );
            kolibri_send_event( get_current_tid(), EV_REDRAW );
            request_update_timer( ks, 50 );
        }
        break;

    case 1: // put pixel
        {
            if( !ks->win )
                break;

            int x = st->ebx;
            int y = st->ecx;
            color_t c = i2color(st->edx); // TODO inverse

            w_draw_pixel( ks->win, x, y, c );
            request_update_timer( ks, REPAINT_TIMEOUT_MS );
        }
        break;

    case 2: // read key
        {
            //u_int32_t e =
            get_event_bits(u,ks); // Need it here to suck in Phantom events
            //if( KOLIBRI_HAS_EVENT_BIT(e, EV_KEY) )

            if(!wtty_is_empty(ks->keys))
            {
                //KOLIBRI_RESET_EVENT_BIT(e, EV_KEY);
                // TODO take mutex
                char ch = wtty_getc(ks->keys); // TODO check read mode - maybe we have to ret scancode
                //ks->have_e = 0; // consume event
                st->eax = 0x0000FF00 & (ch << 8); // have key
                SHOW_FLOW( 1, "read key '%c'", ch);
            }
            else
                st->eax = 1; // no key
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
            color_t bgc = i2color(st->edi);

            int len = st->esi;

            char *str = u_ptr(st->edx, 0);

            int nfont = (st->ecx >> 28) & 0x3;
            int asciiz = (st->ecx >> 30) & 0x1;
            int clear_bg = (st->ecx >> 29) & 0x1;

            bgc.a = clear_bg ? 0xFF : 0;

            //SHOW_FLOW( 2, "string bg", xsize, ysize, xpos, ypos );

            //const drv_video_font_t *font = nfont ? &drv_video_8x16ant_font : &drv_video_8x16cou_font ;
            const drv_video_font_t *font = nfont ? &drv_video_kolibri2_font : &drv_video_kolibri1_font ;

            ypos -= font->ysize;

            if( (!asciiz) && len > 255 )
                break;

            char buf[256+1];

            if( !asciiz )
            {
                strlcpy( buf, str, len );
                str = buf;
            }

            w_font_draw_string( ks->win, font, str, textc, bgc, xpos, ypos );
            request_update_timer( ks, REPAINT_TIMEOUT_MS );
        }
        break;

    case 5: // sleep
        {
            int msec = st->ebx * 10;
            hal_sleep_msec(msec);
        }
        break;

    case 6: // TODO read ramdisk
        {
            char *ifn = u_ptr( st->ebx, 12 ); // NB! No final zero!

            char fn[13];
            strncpy( fn, ifn, 12 );
            fn[12] = 0;

            char *fnp = fn+11;
            while( fnp >= fn )
            {
                if( *fnp <= ' ' ) *fnp = 0;
                else break;
                fnp--;
            }

            int start_block = st->ecx;
            if( start_block <= 0 ) start_block = 1;

            start_block--; // start from 0

            int nblocks = st->edx;
            if( nblocks <= 0 ) nblocks = 1;

            size_t startpos = start_block*512;
            size_t nbytes = nblocks*512;
            size_t after_end_pos = startpos+nbytes;

            char *buf = u_ptr( st->esi, nbytes ); // NB! No final zero!

            SHOW_ERROR( 0, "read ramdisk '%s', start %d, %d blocks", fn, start_block, nblocks );

            // TODO really - read from /sys?

            char fname[256];
            snprintf( fname, sizeof(fname), "/amnt0/%s", fn );

            void *odata;
            size_t osize;
            errno_t rc = k_load_file( &odata, &osize, fname );
            if( rc )
            {
                if( odata ) free(odata);
                st->eax = -1;
                break;
            }

            if( after_end_pos > osize )
            {
                nbytes -= (after_end_pos-osize);
                after_end_pos = osize;
            }

            if( nbytes <= 0 )
            {
                st->eax = -1;
                break;
            }

            memcpy( buf, odata+startpos, nbytes );
            st->eax = nbytes;
        }
        break;

    case 7: // draw image
        {
            rect_t r;

            r.x = st->edx >> 16;
            r.y = st->edx & 0xFFFF;

            r.xsize = st->ecx >> 16;
            r.ysize = st->ecx & 0xFFFF;

            //r.y = ks->win->ysize - r.y;
            r.y = ks->win->ysize - r.y - r.ysize;

            int npixels = r.xsize * r.ysize;

            // st->ebx - BGR bitmap ptr
            const struct rgb_t *src = u_ptr( st->ebx, npixels*3 );

#if 1
            rgba_t *pixels = calloc( sizeof(rgba_t), npixels );
            rgb2rgba_move( pixels, src, npixels );

            bitmap2bitmap_yflip(
                          ks->win->w_pixel, ks->win->xsize, ks->win->ysize, r.x, r.y,
                          pixels, r.xsize, r.ysize, 0, 0, r.xsize, r.ysize );
            free( pixels );
#else
            bitmap2bitmap_yflip(
                          ks->win->pixel, ks->win->xsize, ks->win->ysize, r.x, r.y,
                          src, r.xsize, r.ysize, 0, 0, r.xsize, r.ysize );
#endif
            request_update_timer( ks, REPAINT_TIMEOUT_MS );
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

            //r.y = ks->win->ysize - r.y;
            r.y = ks->win->ysize - r.y - r.ysize;

            int npixels = r.xsize * r.ysize;

            pool_handle_t bh = pool_create_el( ks->buttons, calloc( 1, sizeof(struct kolibri_button) ) );
            if( bh < 0 )
            {
                SHOW_ERROR0( 0, "out of buttons" );
                break;
            }
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
            cb->color = i2color( st->esi );

            // st->ebx - BGR bitmap ptr
            const struct rgb_t *src = u_ptr( st->ebx, npixels*3 );

            cb->pixels = calloc( sizeof(rgba_t), npixels );
            rgb2rgba_move( cb->pixels, src, npixels );

            w_fill_rect( ks->win, cb->color, r );

            if(!cb->flag_nopaint)
            {

                bitmap2bitmap_yflip(
                              ks->win->w_pixel, ks->win->xsize, ks->win->ysize, r.x, r.y,
                              cb->pixels, r.xsize, r.ysize, 0, 0, r.xsize, r.ysize );
            }

            pool_release_el( ks->buttons, bh );
            request_update_timer( ks, REPAINT_TIMEOUT_MS );
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

    case 10: // wait for event forever
        {
            //int rc =
            hal_sem_acquire_etc( &ks->event, 1, 0, 0 );
            /*if(rc)
            {
                st->eax = 0;
                break;
            }*/
            hal_sem_zero( &ks->event ); // eat all available count
        }
        // fall through...
    case 11: // get event bits
        //u_int32_t bits = get_event_bits(u,ks);
        st->eax = eat_event_bit(u,ks);
        break;

    case 12: // begin/and repaint app win
        {
            if( !ks->win )
                break;

            if( st->ebx == 1 )
            {
                ks->win_update_prevent = 1;
                // TODO remove all buttons
                KOLIBRI_RESET_EVENT_BIT(ks->event_bits, EV_REDRAW);
            }

            if( st->ebx == 2 )
            {
                ks->win_update_prevent = 0;
                SHOW_FLOW0( 1, "Repaint" );
                w_update( ks->win );
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

            w_draw_box( ks->win, xpos, ypos, xsize, ysize, c );
            request_update_timer( ks, REPAINT_TIMEOUT_MS );
        }
        break;

    case 14: // get screen size
        {
            u_int32_t xs = 0xFFFF & (scr_get_xsize() - 1);
            u_int32_t ys = 0xFFFF & (scr_get_ysize() - 1);
            st->eax = (xs << 16) | ys;
        }
        break;

    case 17: // get screen button press
        {
            if( ks->pressed_button_id == 0 )
            {
                st->eax = 1; // empty
                break;
            }

            unsigned mb = ks->pressed_button_mouseb;
            SHOW_FLOW( 1, "button mouseb %x", mb );
            mb &= ~1; // kill lower bit

            st->eax = ( (ks->pressed_button_id & 0xFFFFFF) << 8) | (mb&0xFF);
        }
        break;

    case 18: // Info + control
        kolibri_sys_info(u,ks,st);
        break;

    case 20: // play MIDI
        {
            st->eax = 1; // fail
        }
        break;

    case 22: // set date/time
        {
            st->eax = 1; // fail
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

            //int rc =
            hal_sem_acquire_etc( &ks->event, 1, SEM_FLAG_TIMEOUT, 1000*timeout_msec );
            /*if(rc)
            {
                st->eax = 0;
                break;
            }*/
            hal_sem_zero( &ks->event );

            st->eax = eat_event_bit(u,ks);
        }
        break;

    case 24: // CD
        {
            st->eax = 1; // fail
        }
        break;

    case 26: // hardware services
        kolibri_sys_hw(u,ks,st);
        break;

    case 29: // get date
        {
            struct tm *tmp;
            struct tm mytime;
            tmp = current_time;
            mytime = *tmp;

            u_int32_t bcd_d = BCD_BYTE(mytime.tm_mday);
            u_int32_t bcd_m = BCD_BYTE(mytime.tm_mon);
            u_int32_t bcd_y = BCD_BYTE(mytime.tm_year);

            st->eax = (bcd_d << 16) | (bcd_m << 8) | (bcd_y & 0xFF);
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

    case 37: // mouse
        KOLIBRI_RESET_EVENT_BIT(ks->event_bits,EV_MOUSE);
        switch( st->ebx )
        {
        case 0:
            st->eax = COORD_TO_REG(video_drv->mouse_x,scr_get_ysize() - video_drv->mouse_y);
            break;

        case 1:
            {
                int x = video_drv->mouse_x;
                int y = scr_get_ysize() - video_drv->mouse_y;
                if( ks->win )
                {
                    x -= ks->win->x;
                    y -= ks->win->y;
                }
                st->eax = COORD_TO_REG(x,y);
            }
            break;

        case 2:
            st->eax = video_drv->mouse_flags;
            break;

        case 4: // TODO load cursor
            SHOW_ERROR( 0, "Unimplemented mouse 37 syscall ebx = %d ecx = %d", st->ebx, st->ecx );
            st->eax = 0; // failed - must ret cursor handle
            break;

        case 5: // TODO set cursor
            // st->ecx; // cursor handle
            //break;

        case 6: // TODO rm cursor
            // st->ecx; // cursor handle
            SHOW_ERROR( 0, "Unimplemented mouse 37 syscall ebx = %d ecx = %d", st->ebx, st->ecx );
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

            w_draw_line( ks->win, xpos, ypos, xend, yend, c );
            request_update_timer( ks, REPAINT_TIMEOUT_MS );
        }
        break;

    case 40: // set event mask
        ks->event_mask = st->ebx;
        break;

    case 46: // access io ports
        {
            int start_port = st->ecx;
            SHOW_ERROR( 0, "App requested access to io ports %d-%d", start_port, st->edx );
            st->eax = 1; // fail
        }
        break;

    case 47: // print number
        if( !ks->win )
            break;
        {
            //const drv_video_font_t *font = nfont ? &drv_video_8x16ant_font : &drv_video_8x16cou_font ;
            //const drv_video_font_t *font = &drv_video_8x16cou_font;
            const drv_video_font_t *font = &drv_video_kolibri1_font;

            color_t fg = i2color( st->esi );
            color_t bg = i2color( st->edi );

            int fill_bg = st->esi & (1<< 30);

            bg.a = fill_bg ? 0xFF : 0;

            int xpos = st->edx >> 16;
            int ypos = st->edx & 0xFFFF;

            ypos = ks->win->ysize - ypos;
            ypos -= font->ysize;

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


            //SHOW_FLOW( 0, "pnum '%s'", p );

            // TODO clear bg
            w_font_draw_string( ks->win, font, p, fg, bg, xpos, ypos );
            request_update_timer( ks, REPAINT_TIMEOUT_MS );

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

            case 4: // Get skin title bar height - TODO not really implemented
                st->eax = 16;
                break;

            default:
                SHOW_ERROR( 0, "Unimplemented 48 syscall ebx = %d ecx = %d", st->ebx, st->ecx );
                break;
            }
        }
        break;

    case 50: // TODO window shape / scale
        {
            if( !ks->win )
                break;
            if( st->ebx == 0 )
            {
                int sz = (ks->win->xsize >> ks->win_alpha_scale) * (ks->win->ysize >> ks->win_alpha_scale);
                ks->win_user_alpha = u_ptr( st->ecx, sz );
                SHOW_ERROR( 0, "requested alpha map %p", ks->win_user_alpha );
                break;
            }
            if( st->ebx == 1 )
            {
                ks->win_alpha_scale = st->ecx;
                SHOW_ERROR( 0, "requested scale %d", ks->win_alpha_scale );
                break;
            }
            kolibri_reload_window_alpha(ks);
        }
        break;

    case 51: // start thread
        {
            if( st->ebx != 1 )
                break;
            st->eax = kolibri_start_thread( u, ks, st->ecx, st->edx );
        }
        break;

    case 54: // ?
        st->eax = 0x12345678;
        break;

    case 60: // ipc
        {
            switch( st->ebx )
            {

            case 1: // set buf addr
                {
                    st->eax = 0; // success
                    size_t sz = st->edx;
                    void * bp = u_ptr( st->ecx, sz ); // NB! Due to sbrk and others we have to recheck access on real data move
                    // u_ptr may have return on efault, assign below only
                    ks->ipc_buf_addr = bp;
                    ks->ipc_buf_size = sz;
                }
                break;

            case 2: // send msg
                {
                    st->eax = 1;
                    size_t sz = st->esi;
                    void * bp = u_ptr( st->edx, sz ); // NB! Due to sbrk and others we have to recheck access on real data move

                    // TODO take kolibri proc mutex

                    // TODO if dest proc will die during we send msg, kernel will die too

                    pid_t dpid;
                    if(t_get_pid( st->ecx, &dpid ))
                    {
                        st->eax = 4; // no pid
                        break;
                    }

                    uuprocess_t *u = proc_by_pid(dpid);
                    if( 0 == u )
                    {
                        st->eax = 4; // no pid
                        break;
                    }

                    struct kolibri_process_state * dest_ks = get_kolibri_state(u);
                    if( 0 == dest_ks )
                    {
                        st->eax = 4; // no pid
                        break;
                    }

                    kolibri_ipc_buf_t *buf = dest_ks->ipc_buf_addr;
                    size_t bs = dest_ks->ipc_buf_size;

                    if( buf == 0 )
                    {
                        st->eax = 1; // no buf
                        break;
                    }

                    if( buf->busy )
                    {
                        st->eax = 2; // buf busy
                        break;
                    }

                    size_t need_len = sz+sizeof(kolibri_ipc_msg_t);

                    if(need_len > bs-buf->used)
                    {
                        st->eax = 3; // buf overflow
                        break;
                    }

                    kolibri_ipc_msg_t *msg = buf->msg + bs - buf->used;
                    buf->used += need_len;

                    msg->tid = get_current_tid();
                    msg->len = sz;
                    memcpy( msg->data, bp, sz );
                    st->eax = 0; // sent

                    st->eax = 2; // buf busy

                }
                break;
            }

        }
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

    case 64: // sbrk
        {
            // TODO check if ipc buf is still in addr space
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

    case 67: // win move / resize
        {
            if( !ks->win )
                break;

            int x = st->ebx;
            int y = st->ecx;
            int xs =st->edx;
            int ys =st->esi;

            if( (x != -1) && (y != -1) )
                w_move( ks->win, x, y );

            if( (xs != -1) && (ys != -1) )
            {
                // TODO resize - implement
                //w_resize( ks->win, xs, ys );
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

    case 71: // Set win title
        {
            if( !ks->win )
                break;
            if( st->ebx == 1 ) // set title
            {
                const char *title = u_ptr( st->ecx, 0 );
                w_set_title( ks->win, title );
            }
        }
        break;

    default:
        SHOW_ERROR( 0, "Unimplemented Kolibri syscall eax = %d", st->eax );
        st->eax = 2; // Most funcs return nonzero retcode in eax.
    }
}


static void kolibri_send_event( tid_t tid, u_int32_t event_id )
{
    pid_t pid;
    if( t_get_pid( tid, &pid ) )
        return;

    uuprocess_t *u = proc_by_pid(pid);
    if(!u)
        return;

    struct kolibri_process_state * ks = get_kolibri_state(u);
    if(!ks)
        return;

    KOLIBRI_SET_EVENT_BIT( ks->event_bits, event_id );
    hal_sem_release( &ks->event );
}


static u_int32_t get_event_bits(uuprocess_t *u, struct kolibri_process_state *ks)
{
    (void) u;
    u_int32_t bits = ks->event_bits;

    SHOW_FLOW( 11, "try read event, ebits %x", bits );

    // TODO take ks mutex

one_more:
    // TODO here we must check next win q event
    // and set corresp bits
    if( ks->win && (!ks->have_e) )
    {
        ks->have_e = ev_w_get_event( ks->win, &ks->e, 0 );
        SHOW_ERROR( 10, "read event, %s", ks->have_e ? "got" : "none" );
    }

    if( ks->have_e )
    {
        ks->have_e = 0;

        SHOW_FLOW( 2, "got event type %d, ikep %p", ks->e.type, ks->win->inKernelEventProcess );
        switch( ks->e.type )
        {
        case UI_EVENT_TYPE_MOUSE:
            SHOW_FLOW0( 10, "mou event" );
            KOLIBRI_SET_EVENT_BIT(bits, EV_MOUSE);
            if( ks->e.m.buttons )
                kolibri_check_button( ks->e.rel_x, ks->e.rel_y, ks, ks->e.m.buttons );

            kolibri_paint_changed_buttons(ks);

            break;

        case UI_EVENT_TYPE_KEY:
            SHOW_FLOW0( 10, "key event" );
            //KOLIBRI_SET_EVENT_BIT(bits, EV_KEY);

            if( ks->e.modifiers & UI_MODIFIER_KEYUP ) // Right? Or shall we give it to Kolibri app too?
            {
                SHOW_FLOW( 10, "skip keyup event type %d", ks->e.type );
                goto one_more;
            }

            if( wtty_putc_nowait(ks->keys, ks->e.k.ch) )
                SHOW_ERROR0( 1, "key lost" );
            break;

        default:
            // some unknown stuff - get one more Phantom event
            //SHOW_FLOW( 2, "skip event type %d", ks->e.type );

            ks->defaultEventProcess(ks->win, &ks->e);

            goto one_more;
        }
    }

    if(!wtty_is_empty(ks->keys))
        KOLIBRI_SET_EVENT_BIT(bits, EV_KEY);

    SHOW_FLOW( 10, "event bits %x, mask %x, out bits %x", bits, ks->event_mask, bits & ks->event_mask );

    return bits & ks->event_mask;
}


static u_int32_t eat_event_bit(uuprocess_t *u, struct kolibri_process_state *ks)
{
    u_int32_t bits = get_event_bits(u,ks);
    if( bits == 0 ) return 0;

    int bitno = ffs(bits);
    if( bitno == 0 ) return 0;
    KOLIBRI_RESET_EVENT_BIT(ks->event_bits,bitno);

    SHOW_FLOW( 10, "event bit no %d", bitno );

    return bitno;
}



void kolibri_thread_starter( void * arg );

static tid_t kolibri_start_thread( uuprocess_t *u, struct kolibri_process_state *ks, addr_t eip, addr_t esp )
{
    (void) u;
    (void) ks;

    struct kolibri_thread_start_parm *sp = calloc(1,sizeof(struct kolibri_thread_start_parm));
    if( sp == 0 )
    {
        SHOW_ERROR0( 0, "out of mem" );
        return -1;
    }

    sp->eip = eip;
    sp->esp = esp;

    // todo check esp/eip sanity?
    tid_t tid = hal_start_thread( kolibri_thread_starter, (void *)sp, THREAD_FLAG_USER );

    return tid > 0 ? tid : -1;
}


static void kolibri_sys_hw( uuprocess_t *u, struct kolibri_process_state * ks, struct trap_state *st )
{
    (void) u;
    (void) ks;

    switch(st->ebx)
    {
    case 1:
        st->eax = 0x330; // ? MPU port :)
        break;

    case 2: // keyb map/lang
        if(st->ecx == 9)
            st->eax = 1; // en
        else
            goto unknown;
        break;

    case 3: // CD dev
    case 7: // HD dev
    case 8: // HD part
        st->eax = 0; // None
        break;

    case 5: // lang
        st->eax = 1; // en
        break;

    case 9: // uptime in 0.1 sec
        {
            bigtime_t upt = hal_system_time(); // uptime, microsecs

            upt /= 100000; // -> 0.1 sec

            st->eax = (u_int32_t)upt; // None
        }
        break;

    case 11: // is low level HDD access enabled?
    case 12: // is low level PCI access enabled?
        st->eax = 0; // No
        break;

    default:
    unknown:
        SHOW_ERROR( 0, "Unimplemented Kolibri 26 syscall ebx = %d", st->ebx );
        st->eax = 2; // Most funcs return nonzero retcode in eax.
    }
}





static void kolibri_reload_window_alpha(struct kolibri_process_state *ks)
{
    int sz = (ks->win->xsize >> ks->win_alpha_scale) * (ks->win->ysize >> ks->win_alpha_scale);
    int npixel = ks->win->xsize * ks->win->ysize;
    rgba_t *pp = ks->win->w_pixel;

    int scaled_sz = sz << ks->win_alpha_scale*2;

    if( scaled_sz != npixel )
    {
        SHOW_ERROR( 1, "scale %d, npixel %d, sclaed sz %d, sz %d - window size does not match scale", ks->win_alpha_scale, npixel, scaled_sz, sz );
        return;
    }

    int alpha_map_stride = ks->win->xsize >> ks->win_alpha_scale;

    if( ks->win_user_alpha == 0 )
    {
        while(npixel-- > 0)
        {
            pp->a = 0xFF;
            pp++;
        }
        return;
    }

    int x, y;
    for( y = 0; y < ks->win->ysize; y++ )
    {
        for( x = 0; x < ks->win->xsize; x++ )
        {
            u_int8_t alpha_byte = ks->win_user_alpha[ (x >> ks->win_alpha_scale) + (ks->win->ysize-y-1) * alpha_map_stride ];
            pp->a = alpha_byte ? 0xFF : 0;
            pp++;
        }
    }
}






























#endif // HAVE_KOLIBRI
