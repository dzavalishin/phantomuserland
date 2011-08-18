/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Intel i810 video card driver. Incomplete.
 *
 *
**/

#ifdef ARCH_ia32

#define DEBUG_MSG_PREFIX "video-intel"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include <hal.h>
#include <kernel/vm.h>
#include <kernel/device.h>
#include <ia32/phantom_pmap.h>
#include <kernel/page.h>

#include <ia32/pio.h>
#include <kernel/bus/pci.h>
#include <phantom_libc.h>

#include <video.h>
#include <video/screen.h>

static int i810_video_probe();
static int i810_video_start();
static int i810_video_stop();

static int i810_init();



struct drv_video_screen_t        video_driver_i810 =
{
    "Intel810",
    // size
    800, 600, 32,
    // mouse x y flags
    0, 0, 0,

    // screen
screen:			0,

probe: 			i810_video_probe,
start: 			i810_video_start,
stop:   		i810_video_stop,

update: 		drv_video_null,
bitblt: 		drv_video_bitblt_rev,
winblt:			drv_video_win_winblt_rev,
readblt: 		drv_video_readblt_rev,

mouse:    		drv_video_null,

redraw_mouse_cursor: 	drv_video_draw_mouse_deflt,
set_mouse_cursor: 	drv_video_set_mouse_cursor_deflt,
mouse_disable:          drv_video_mouse_off_deflt,
mouse_enable:          	drv_video_mouse_on_deflt,

};


typedef struct {
    int         dummy;
} i810;

static phantom_device_t * dev;
static i810 * vcard = NULL;

static int n_pages = 1024;


static int seq_number = 0;
phantom_device_t * driver_intel_810_pci_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    if( seq_number )
    {
        SHOW_ERROR0( 0, "Just one");
        return 0;
    }


    SHOW_FLOW0( 1, "probe" );

    vcard = calloc(1, sizeof(i810));
    dev = malloc(sizeof(phantom_device_t));
    if( (vcard == 0) || (dev == 0) )
    {
        SHOW_ERROR0( 0, "out of mem");
        return 0;
    }

    dev->irq = pci->interrupt;

    int i;
    for (i = 0; i < 6; i++)
    {
        if (pci->base[i] > 0xffff)
        {
            dev->iomem = pci->base[i];
            dev->iomemsize = pci->size[i];

            n_pages = dev->iomemsize/PAGE_SIZE; // FIXME partial page

            SHOW_INFO( 0,  "base 0x%lx, size 0x%lx", dev->iomem, dev->iomemsize );
        } else if( pci->base[i] > 0) {
            dev->iobase = pci->base[i];
            SHOW_INFO( 0,  "io_port 0x%x", dev->iobase );
        }
    }

    SHOW_FLOW0( 1, "init");
    if (i810_init() < 0)
    {
        SHOW_ERROR0( 0, "init failed");
        goto free;
    }


    dev->name = "i810";
    dev->seq_number = seq_number++;
    dev->drv_private = vcard;


    return dev;

free:
    free(vcard);
    free(dev);
    return 0;
}



static int i810_init()
{
    return 0;
}




static int i810_video_probe()
{
    if(!seq_number)
        return 0;

    if( hal_alloc_vaddress((void **)&video_driver_i810.screen, n_pages) )
        panic("Can't alloc vaddress for %d videmem pages", n_pages);

    SHOW_FLOW( 7, "vmem va 0x%X", video_driver_i810.screen);


    //printf("Intel board 0x%x found\n", id);
    return 1;
}






static void i810_map_video(int on_off)
{
    assert( video_driver_i810.screen != 0 );

    hal_pages_control_etc(
                          dev->iomem,
                          video_driver_i810.screen,
                          n_pages, on_off ? page_map : page_unmap, page_rw,
                          INTEL_PTE_WTHRU|INTEL_PTE_NCACHE );
}

static int i810_video_start()
{
    i810_map_video( 1 );
    return 0;
}

static int i810_video_stop()
{
    i810_map_video(0);
    
    video_drv_basic_vga_set_text_mode();
    return 0;
}




#endif // ARCH_ia32




























