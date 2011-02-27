
#define DEBUG_MSG_PREFIX "null"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/drivers.h>
#include <phantom_libc.h>

//#include <hal.h>




// Stop device
static int null_stop(phantom_device_t *dev)
{
    (void) dev;
    return 0;
}



static int null_read(struct phantom_device *dev, void *buf, int len)
{
    (void) dev;
    memset( buf, 0, len );
    return len;
}

static int null_write(struct phantom_device *dev, const void *buf, int len)
{
    (void) dev;
    (void) buf;
    return len;
}



// TODO this is to be moved to driver_map.c and be kept in driver tables
static int seq_number = 0;

phantom_device_t * driver_null_probe( char *name, int stage )
{
    phantom_device_t * dev;

    (void) name;
    (void) stage;

    if(seq_number)        return 0; // just one instance!

    dev = malloc(sizeof(phantom_device_t));
    dev->name = "null";
    dev->seq_number = seq_number++;

    dev->dops.stop = null_stop;

    dev->dops.read = null_read;
    dev->dops.write = null_write;

    return dev;
}




