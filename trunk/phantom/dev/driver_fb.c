
#define DEBUG_MSG_PREFIX "fb"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/drivers.h>
#include <phantom_libc.h>
#include <sys/ioctl.h>


static int 			have_w = 0;
static drv_video_window_t *	w = 0;

static rect_t r =
{
    .xsize = 320,
    .ysize = 200,

    .x = 100,
    .y = 450
};

static void fb_init_window();




static int fb_start(phantom_device_t *dev)
{
    (void) dev;
    fb_init_window();
    return 0;
}


// Stop device
static int fb_stop(phantom_device_t *dev)
{
    (void) dev;
    return 0;
}



static int fb_read(struct phantom_device *dev, void *buf, int len)
{
    (void) dev;

    if(!have_w)
        fb_start(dev);

    if(!have_w)
        return -1;

    memset( buf, 0, len );
    return len;
}

static int fb_write(struct phantom_device *dev, const void *buf, int len)
{
    (void) dev;
    (void) buf;

    if(!have_w)
        fb_start(dev);

    if(!have_w)
        return -1;

    int max = w->xsize * w->ysize * sizeof(rgba_t);
    if( len > max ) len = max;

    memcpy( w->pixel, buf, len );

    drv_video_winblt( w );

    return len;
}

static int fb_ioctl(struct phantom_device *dev, int type, void *buf, int len)
{
    switch(type)
    {
    case IOCTL_FB_SETBOUNDS:
        {
            if( len < sizeof(rect_t) )
                return EINVAL;

            // todo sanity check
            r = *(rect_t *)buf;

            SHOW_FLOW( 1, "pos %d/%d, size %d/%d", r.x, r.y, r.xsize, r.ysize );
            fb_init_window();
        }

    default:
        return ENOSYS;
    }
}



// TODO this is to be moved to driver_map.c and be kept in driver tables
static int seq_number = 0;

phantom_device_t * driver_framebuf_probe( const char *name, int stage )
{
    phantom_device_t * dev;

    (void) name;
    (void) stage;

    if(seq_number)        return 0; // just one instance!

    dev = malloc(sizeof(phantom_device_t));
    dev->name = "fb";
    dev->seq_number = seq_number++;

    dev->dops.stop = fb_start;
    dev->dops.stop = fb_stop;

    dev->dops.read = fb_read;
    dev->dops.write = fb_write;

    dev->dops.ioctl = fb_ioctl;

    return dev;
}
























static void fb_init_window()
{
    if( 0 != w )
        drv_video_window_free(w);

    w = 0;

    w = drv_video_window_create( r.xsize, r.ysize,
                        r.x, r.y, COLOR_BLACK, "FrameBuf" );

    if( 0 == w )
        return;

    //w->owner = GET_CURRENT_THREAD();

    have_w = 1;
}


















