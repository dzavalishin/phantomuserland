/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Protected mode BOCHS virtual video card driver.
 *
 *
**/

#ifdef ARCH_ia32

#define DEV_NAME "Bochs VGA PCI driver"

#define DEBUG_MSG_PREFIX "vga.bochs"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <hal.h>
#include <kernel/vm.h>
#include <kernel/drivers.h>
#include <kernel/page.h>

#include <ia32/phantom_pmap.h>

#include <ia32/pio.h>
#include <phantom_libc.h>

//#include <video.h>
#include <video/screen.h>
#include <video/internal.h>

#include "video_drv_bochs_vbe.h"

static int bochs_video_probe();
static int bochs_video_start();
static int bochs_video_stop();

static physaddr_t bochs_iomem_phys = 0;
static physaddr_t bochs_framebuf_phys = 0;
static int bochs_framebuf_phys_pages = 1024;

// TODO - try 32bpp?

struct drv_video_screen_t        video_driver_bochs_vesa_emulator =
{
    "Bochs VBE emulator",
    // size
    BOCHS_VIDEO_DRV_DEFAULT_X_SIZE, BOCHS_VIDEO_DRV_DEFAULT_Y_SIZE,
    // bpp
    24,
    // mouse x y flags
    0, 0, 0,

    // screen
screen:			0,

probe: 			bochs_video_probe,
start: 			bochs_video_start,
stop:   		bochs_video_stop,

#if 0
update: 		vid_null,
bitblt: 		drv_video_bitblt_rev,
winblt:			drv_video_win_winblt_rev,
readblt: 		drv_video_readblt_rev,
bitblt_part:            drv_video_bitblt_part_rev,
//#endif

mouse:    		vid_null,

//#if 0
redraw_mouse_cursor: 	drv_video_draw_mouse_deflt,
set_mouse_cursor: 	drv_video_set_mouse_cursor_deflt,
mouse_disable:          drv_video_mouse_off_deflt,
mouse_enable:          	drv_video_mouse_on_deflt,
#endif

};





static void vbe_write(unsigned short index, unsigned short value)
{
   outw(VBE_DISPI_IOPORT_INDEX, index);
   outw(VBE_DISPI_IOPORT_DATA, value);
}

static int vbe_read(unsigned short index)
{
    outw(VBE_DISPI_IOPORT_INDEX, index);
    return inw(VBE_DISPI_IOPORT_DATA);
}


// Return nonzero on err
int vbe_set(unsigned short xres, unsigned short yres, unsigned short bpp)
{
    vbe_write(VBE_DISPI_INDEX_ID, VBE_DISPI_ID2);
    int id = vbe_read(VBE_DISPI_INDEX_ID);

    printf("VBE ver 0x%x... ", id);
    //getchar();

    if( id < VBE_DISPI_ID2 || id > VBE_DISPI_ID3 )
        return 1;

    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_write(VBE_DISPI_INDEX_XRES, xres);
    vbe_write(VBE_DISPI_INDEX_YRES, yres);
    vbe_write(VBE_DISPI_INDEX_BPP, bpp);
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    return 0;
}

static int bochs_video_probe()
{

    //printf("Probing for ");
    vbe_write(VBE_DISPI_INDEX_ID, VBE_DISPI_ID2);
    int id = vbe_read(VBE_DISPI_INDEX_ID);

    printf("Bochs VBE emulator id 0x%x \n", id);

    if( id < VBE_DISPI_ID2 || id > VBE_DISPI_ID3 )
        return VIDEO_PROBE_FAIL;

    if( hal_alloc_vaddress((void **)&video_driver_bochs_vesa_emulator.screen, bochs_framebuf_phys_pages) )
        panic("Can't alloc vaddress for %d videmem pages", bochs_framebuf_phys_pages);

    SHOW_FLOW( 7, "vmem va 0x%X", video_driver_bochs_vesa_emulator.screen);

    if( bochs_framebuf_phys == 0 )
    {
        SHOW_ERROR0( 0, "Unknown physmem addr for frame buffer" );
        return VIDEO_PROBE_FAIL;
    }

    printf("Bochs VBE emulator ver 0x%x found\n", id);
    return VIDEO_PROBE_SUCCESS;
}




#define VBE_DISPI_LFB_PHYSICAL_ADDRESS  0xE0000000


static void bochs_map_video(int on_off)
{
    assert( video_driver_bochs_vesa_emulator.screen != 0 );

    if( bochs_framebuf_phys == 0 )
    {
        SHOW_ERROR0( 0, "Unknown physmem addr for frame buffer" );
        return;
    }

    hal_pages_control_etc(
                          //VBE_DISPI_LFB_PHYSICAL_ADDRESS,
                          bochs_framebuf_phys,
                          video_driver_bochs_vesa_emulator.screen,
                          bochs_framebuf_phys_pages, on_off ? page_map : page_unmap, page_rw,
                          INTEL_PTE_WTHRU|INTEL_PTE_NCACHE );
}

static int bochs_video_start()
{
    bochs_map_video( 1 );
    return vbe_set(BOCHS_VIDEO_DRV_DEFAULT_X_SIZE, BOCHS_VIDEO_DRV_DEFAULT_Y_SIZE, 24);
}

static int bochs_video_stop()
{
    bochs_map_video(0);
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    video_drv_basic_vga_set_text_mode();
    return 0;
}



phantom_device_t * driver_bochs_svga_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    SHOW_FLOW( 1, "Probe for " DEV_NAME " stage %d", stage );

    phantom_device_t * dev = calloc(sizeof(phantom_device_t),1);

    int i;
    for (i = 0; i < 6; i++)
    {
        if (pci->base[i] > 0xffff)
        {
            SHOW_INFO( 1, "mem base 0x%lx, size 0x%lx", dev->iomem, dev->iomemsize);

            if( pci->size[i] <= 0x2000 ) 
            {
                bochs_iomem_phys = pci->base[i];

                dev->iomem = pci->base[i];
                dev->iomemsize = pci->size[i];
            }
            else 
            {
                bochs_framebuf_phys = pci->base[i];
                bochs_framebuf_phys_pages = BYTES_TO_PAGES(pci->size[i]);
            }

        } else if( pci->base[i] > 0) {
            dev->iobase = pci->base[i];
            SHOW_INFO( 1, "io_port 0x%x", dev->iobase);
        }
    }

    dev->irq = pci->interrupt;

    // Gets port 0. uninited by BIOS? Need explicit PCI io addr assign?
    if( dev->iomem == 0 )
    {
        SHOW_ERROR0( 0, "No io mem?" );
        goto free;
    }

    SHOW_FLOW( 1, "Got " DEV_NAME " at io mem %X framebuf %x, %d pages", 
        bochs_iomem_phys, bochs_framebuf_phys, bochs_framebuf_phys_pages );
    
    dev->name = DEV_NAME;
    dev->seq_number = 0;

    /*
    if( hal_irq_alloc( dev->irq, &es1370_interrupt, dev, HAL_IRQ_SHAREABLE ) )
    {
        SHOW_ERROR( 0, "IRQ %d is busy", dev->irq );
        goto free;
    }*/


    dev->drv_private = &video_driver_bochs_vesa_emulator;

    return dev;

free:
    free(dev);
    return 0;
}



#endif // ARCH_ia32






