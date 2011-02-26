/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Console redirection.
 *
**/

#include <phantom_libc.h>
#include <drv_video_screen.h>
#include <kernel/debug.h>

#include <console.h>

// This is a very simple console switch/redirection tool



int 	null_set_color(struct rgba_t c) {     (void) c; return 0; 	}

static struct console_ops default_ops =
{
    .getchar 		= phantom_scan_console_getc,
    .putchar 		= driver_isa_vga_putc,
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

    if(ops->putchar) return ops->putchar(c);
    // No way to handle :(
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

        return ops->puts(s);
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


void    console_set_message_color()     { console_set_fg_color( COLOR_LIGHTGREEN ); }
void    console_set_error_color()       { console_set_fg_color( COLOR_LIGHTRED ); }
void    console_set_warning_color()     { console_set_fg_color( COLOR_YELLOW ); }
void    console_set_normal_color()      { console_set_fg_color( COLOR_LIGHTGRAY ); }


