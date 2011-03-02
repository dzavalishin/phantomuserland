/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Default console driver.
 *
 *
**/


#define DEBUG_MSG_PREFIX "console"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/drivers.h>
#include <phantom_libc.h>

//#include <hal.h>




// Stop device
static int console_stop(phantom_device_t *dev)
{
    (void) dev;
    return 0;
}



static int console_read(struct phantom_device *dev, void *buf, int len)
{
    (void) dev;
    char *cp = buf;
    int nw = len;
    int nread = 0;

    while(nw--)
    {
        *cp++ = getchar();
        nread++;

        if( *cp == '\n' )
            break;
    }

    return nread;
}

static int console_write(struct phantom_device *dev, const void *buf, int len)
{
    (void) dev;
    const char *cp = buf;
    int nw = len;

    while(nw--)
    {
        putchar(*cp++);
    }

    return len;
}



// TODO this is to be moved to driver_map.c and be kept in driver tables
static int seq_number = 0;

phantom_device_t * driver_console_probe( const char *name, int stage )
{
    phantom_device_t * dev;

    (void) name;
    (void) stage;

    if(seq_number)        return 0; // just one instance!

    dev = malloc(sizeof(phantom_device_t));
    dev->name = "console";
    dev->seq_number = seq_number++;

    dev->dops.stop = console_stop;

    dev->dops.read = console_read;
    dev->dops.write = console_write;

    return dev;
}




