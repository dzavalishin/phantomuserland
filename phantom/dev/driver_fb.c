
#define DEBUG_MSG_PREFIX "fb"
#include <debug_ext.h>
#define debug_level_flow 7
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/drivers.h>
#include <phantom_libc.h>
#include <sys/ioctl.h>
#include <video/color.h>
#include <video/point.h>
#include <video/rect.h>
#include <event.h>


static int              have_w = 0;
static window_handle_t  w = 0;
static color_t			color;

static rect_t wbox =
{
    .xsize = 320,
    .ysize = 200,

    .x = 100,
    .y = 450
};

static void fb_init_window();
static int fb_ioctl(struct phantom_device *dev, int type, void *buf, int len);




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


    //SHOW_FLOW( 1, "rd %d", len );

    if(!have_w)
        fb_start(dev);

    if(!have_w)
        return -1;

    if( len < sizeof(ui_event_t) )
        return EINVAL;

    len = sizeof(ui_event_t);

    ui_event_t e;
    int wait = 0; // TODO fcntl

    //SHOW_FLOW( 1, "do rd %d", len );

again:
    if( 0 == drv_video_window_get_event( w, &e, wait ) )
        return 0;

    if(e.type == UI_EVENT_TYPE_KEY)
        SHOW_FLOW( 8, "got key e %x/%x '%c'", e.k.vk, e.k.ch, e.k.ch );
    else
        SHOW_FLOW( 8, "got e type %d", e.type );
    if(
       (e.type == UI_EVENT_TYPE_WIN) ||
       (e.type == UI_EVENT_TYPE_GLOBAL)
      )
    {
        defaultWindowEventProcessor( w, &e );
        goto again;
    }

    memcpy( buf, &e, len );

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

    drv_video_window_update( w );

    return len;
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

    w = drv_video_window_create( wbox.xsize, wbox.ysize,
                        wbox.x, wbox.y, COLOR_BLACK, "FrameBuf" );

    if( 0 == w )
        return;

    //w->owner = GET_CURRENT_THREAD();
    color = COLOR_WHITE;

    w->inKernelEventProcess = 0;

    have_w = 1;
}


#define CHECK_RECT() if( len < sizeof(rect_t) )   return EINVAL
#define CHECK_POINT() if( len < sizeof(point_t) )   return EINVAL

static int fb_ioctl(struct phantom_device *dev, int type, void *buf, int len)
{
    rect_t r;

    switch(type)
    {
    case IOCTL_FB_FLUSH:
        drv_video_window_update( w );
        break;

    case IOCTL_FB_SETBOUNDS:
        {
            CHECK_RECT();

            // todo sanity check
            wbox = *(rect_t *)buf;

            SHOW_FLOW( 2, "pos %d/%d, size %d/%d", wbox.x, wbox.y, wbox.xsize, wbox.ysize );
            fb_init_window();
        }
        break;

    case IOCTL_FB_SETCOLOR:
        {
            if( len < sizeof(color_t) )
                return EINVAL;
            color = *(color_t *)buf;
        }
        break;


    case IOCTL_FB_DRAWBOX:
        {
            CHECK_RECT();

            // todo sanity check
            r = *(rect_t *)buf;

            SHOW_FLOW( 9, "draw box %d/%d, size %d/%d", r.x, r.y, r.xsize, r.ysize );
            drv_video_window_draw_box( w, r.x, r.y, r.xsize, r.ysize, color);
        }
        break;

    case IOCTL_FB_FILLBOX:
        {
            CHECK_RECT();

            // todo sanity check
            r = *(rect_t *)buf;

            SHOW_FLOW( 9, "fill box %d/%d, size %d/%d", r.x, r.y, r.xsize, r.ysize );
            drv_video_window_fill_box( w, r.x, r.y, r.xsize, r.ysize, color);
        }
        break;

    case IOCTL_FB_DRAWLINE:
        {
            CHECK_RECT();

            // todo sanity check
            r = *(rect_t *)buf;

            r.xsize += r.x;
            r.ysize += r.y;

            SHOW_FLOW( 9, "draw line %d/%d, size %d/%d", r.x, r.y, r.xsize, r.ysize );
            drv_video_window_draw_box( w, r.x, r.y, r.xsize, r.ysize, color);
        }
        break;

    case IOCTL_FB_DRAWPIXEL:
        {
            CHECK_POINT();
            point_t p = *(point_t *)buf;
            drv_video_window_pixel ( w, p.x, p.y, color );
        }
        break;


    default:
        return ENOSYS;
    }

    return 0;
}














