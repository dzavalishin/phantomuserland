#define DEBUG_MSG_PREFIX "video"
#include "debug_ext.h"
static int debug_level_flow = 2;

#include <phantom_libc.h>

#include "video.h"
#include "hal.h"

#include "misc.h"


// TODO: panic must switch to text mode!

// Placeholder for absent drv methods
//void drv_video_null() {}


#define JUST_VGA 0

struct drv_video_screen_t *video_drivers[] =
{
    &video_driver_basic_vga,
#if !JUST_VGA
    &video_driver_cirrus,
    &video_driver_bochs_vesa_emulator,
    // test one. never reports success
    &video_driver_direct_vesa,
#if !VESA_ENFORCE
    &video_driver_bios_vesa,
#endif
#endif
};


struct drv_video_screen_t        *video_drv = 0;

static void phantom_select_video_driver()
{
    long selected_sq = 0;
    struct drv_video_screen_t *selected_drv = NULL;

    SHOW_FLOW0( 2, "Look for video driver" );

    unsigned int i;
    for( i = 0; i < (sizeof(video_drivers)/sizeof(struct drv_video_screen_t *)); i++ )
    {
        struct drv_video_screen_t *drv = video_drivers[i];

        SHOW_FLOW( 2, "Probing %s video driver: ", drv->name);
        if( !drv->probe() )
        {
            SHOW_FLOW( 2, "Video driver %s : No", drv->name);
            continue;
        }
        SHOW_FLOW( 2, "Video driver %s : Yes", drv->name);

        long sq = drv->xsize * drv->ysize;

        if( sq > selected_sq )
        {
            selected_drv = drv;
            selected_sq = sq;
        }
    }

    if( selected_drv == NULL )
    {
        printf("No video driver found!");
    }
    else
    {
        if(video_drv != NULL)
            video_drv->stop();

        SHOW_FLOW( 1, "The best is %s video driver\n", selected_drv->name);
        video_drv = selected_drv;
    }
}

static int was_enforced = 0;


static void video_post_start()
{
#if VIDEO_ZBUF
    video_zbuf_init();
#endif
    phantom_init_console_window();
    drv_video_set_mouse_cursor(drv_video_get_default_mouse_bmp());
}


void phantom_start_video_driver(void)
{
    if( was_enforced && video_drv != 0 )
    {
        SHOW_FLOW0( 1, "Skipping video drv select due to enforce" );
        return;
    }

    phantom_select_video_driver();
//getchar();
    int res = 1;
//getchar();
    if(video_drv) res = video_drv->start();
    // TODO if start fails, mark driver as not working and select again

    if( res )
        panic("I don't know how to work with this video hardware, sorry");
    video_post_start();
}

void phantom_stop_video_driver()
{
    phantom_stop_console_window();
    if(video_drv) video_drv->stop();
}

// TODO start/stop - we can stop unstarted drivers - fix
void phantom_enforce_video_driver(struct drv_video_screen_t *vd)
{
    if(video_drv)
        video_drv->stop();

    video_drv =  vd;

    int res;
    if(video_drv) res = video_drv->start();
    // TODO if start fails, mark driver as not working and select again

    if( res )
        panic("I don't know how to work with this video hardware, sorry");

    was_enforced = 1;
    video_post_start();
}

