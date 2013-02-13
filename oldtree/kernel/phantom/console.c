/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Console redirection.
 *
**/

#define DEBUG_MSG_PREFIX "con"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include <phantom_libc.h>
//#include <drv_video_screen.h>
#include <kernel/init.h>
#include <kernel/libkern.h>
#include <kernel/debug.h>
#include <kernel/interrupts.h>
#include <kernel/board.h>

#include <video/window.h>


#include <console.h>

// GET_CPU_ID()
#include <kernel/smp.h>

// This is a very simple console switch/redirection tool


static void dmesg_putchar(int c);
static void dmesg_puts(const char *s);



int 	null_set_color(struct rgba_t c) {     (void) c; return 0; 	}

static struct console_ops default_ops =
{
    .getchar 		= board_boot_console_getc, // phantom_scan_console_getc,
    .putchar 		= board_boot_console_putc, // driver_isa_vga_putc,
    .puts       	= 0,
    .set_fg_color       = null_set_color,
    .set_bg_color       = null_set_color,
};


static struct console_ops       *ops = &default_ops;
static struct console_ops       run_ops;
static struct console_ops       old_ops;


void phantom_set_console_ops( struct console_ops *in_ops )
{
    old_ops = *ops;
    ops = &old_ops;

    run_ops = *in_ops;

    if(!run_ops.getchar)       	run_ops.getchar      = old_ops.getchar;
    if(!run_ops.set_fg_color) 	run_ops.set_fg_color = null_set_color;
    if(!run_ops.set_bg_color) 	run_ops.set_bg_color = null_set_color;

    ops = &run_ops;
}


static volatile int console_reenter = 0;

int getchar(void)
{
    if(ops->getchar) return ops->getchar();

    printf("getchar() called before we got a driver");
    return -1;
}

int putchar(int c)
{
	// Send a copy to serial port or whatever...
    debug_console_putc(c);

    dmesg_putchar(c);

    int glock = 1;
#if !HAVE_SMP
    glock = global_lock_entry_count[GET_CPU_ID()] == 0;
#endif

    console_reenter++;
    if( (!IN_INTERRUPT()) && glock && (console_reenter==1) )
        if(ops->putchar)
            ops->putchar(c);
    // No way to handle :(
    console_reenter--;

    return c;
}


int
puts(const char *s)
{
    if(ops->puts)
    {
        const char *sc = s;
        while(*sc)
            debug_console_putc(*sc++);

        dmesg_puts(s);

        int rc = 0;

        console_reenter++;
        if( (!IN_INTERRUPT()) && (console_reenter == 1) )
            rc = ops->puts(s);
        console_reenter--;

        return rc;
    }

    while(*s)
        putchar(*s++);
    return 0;
}


void phantom_set_console_getchar( int (*_getchar_impl)(void) )
{
    ops->getchar = _getchar_impl;
}


void phantom_set_console_putchar( int (*_putchar_impl)(int) )
{
    ops->putchar = _putchar_impl;
}

void phantom_set_console_puts( int (*_puts_impl)(const char *) )
{
    ops->puts = _puts_impl;
}


void console_set_fg_color( struct rgba_t fg )
{
    if(ops->set_fg_color) ops->set_fg_color(fg);
}


// -----------------------------------------------------------------------
// dmesg buffer
// -----------------------------------------------------------------------


#define DMESG_BS (1024*32)

static hal_spinlock_t dm_spin;

static char dm_buf[DMESG_BS];

static char *dm_pp = dm_buf; // put ptr
static char *dm_gp = dm_buf; // get ptr

static size_t dm_rlen = 0;

static void dmesg_init(void)
{
    hal_spin_init( &dm_spin );
}

INIT_ME( dmesg_init, 0, 0 );

int dmesg_read_buf( char *out, size_t olen, off_t start )
{
    //int ie = hal_save_cli();
    hal_spin_lock_cli( &dm_spin );

    if( start == 0 )
    {
        dm_gp = dm_pp;
        dm_rlen = DMESG_BS;

        // Skip empty part if buffer is not full yet
        while( (*dm_gp == 0) && (dm_gp < (dm_buf+DMESG_BS)) )
            dm_gp++;

    }

    size_t len = umin( olen, dm_rlen );

    // We can't! It calls us! Recurred spin!
    //SHOW_FLOW( 1, "rd %d", len );

    if( dm_gp+len > (dm_buf+DMESG_BS))
    {
        int part = dm_buf+DMESG_BS-dm_gp;

        strncpy( out, dm_gp, part );
        dm_rlen -= part;
        len     -= part;
        dm_gp   = dm_buf;
    }

    strncpy( out, dm_gp, len );
    dm_rlen -= len;
    dm_gp   += len;

    hal_spin_unlock_sti( &dm_spin );
    //if( ie ) hal_sti();

    return len;
}



static void dmesg_wrap_pp(void)
{
    if(dm_pp >= dm_buf+DMESG_BS)
        dm_pp = dm_buf;
}

static void dmesg_putchar(int c)
{
    //int ie = hal_save_cli();
    hal_spin_lock_cli( &dm_spin );
    dmesg_wrap_pp();
    *dm_pp++ = c;
    hal_spin_unlock_sti( &dm_spin );
    //if( ie ) hal_sti();
}

static void dmesg_puts(const char *s)
{
    //int ie = hal_save_cli();
    hal_spin_lock_cli( &dm_spin );

    int maxput = DMESG_BS/2;

    while( (*s) && maxput-- > 0 )
    {
        dmesg_wrap_pp();
        *dm_pp++ = *s++;
    }

    hal_spin_unlock_sti( &dm_spin );
    //if( ie ) hal_sti();
}










