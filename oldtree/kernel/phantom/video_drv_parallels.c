/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2017 Dmitry Zavalishin, dz@dz.ru
 *
 * Parallels 'SVGA' driver. Unfinished.
 *
**/

#ifdef ARCH_ia32

#define DEBUG_MSG_PREFIX "parallels.vga"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <hal.h>
#include <errno.h>
#include <device.h>

#include <ia32/pio.h>

#include <kernel/bus/pci.h>

#include <kernel/page.h>
#include <kernel/atomic.h>
#include <kernel/libkern.h>
#include <phantom_libc.h>
#include <video/screen.h>
#include <video/internal.h>


/*

Видеомоду можно выставить так:
               WRITE_PORT_UCHAR((UCHAR *)0x3c4, 0xa9);
               WRITE_PORT_UCHAR((UCHAR *)0x3c5, (UCHAR)Bpp ); // 8/15/16/24/32 
               WRITE_PORT_USHORT((USHORT *)0x3c5, (USHORT) Width);
               WRITE_PORT_USHORT((USHORT *)0x3c5, (USHORT) Height);
               WRITE_PORT_USHORT((USHORT *)0x3c5, (USHORT) BytesPerLine);
               WRITE_PORT_USHORT((USHORT *)0x3c5, (USHORT) RefreshRate ); // value is ignored 
               WRITE_PORT_UCHAR((UCHAR *)0x3c5, 1 ); // enable SVGA
               WRITE_PORT_ULONG((ULONG *)0x3c5, 0 ); //  framebuffer offset 
 
Вернуть текстовый режим:
               WRITE_PORT_UCHAR((UCHAR *)0x3c4, 0xae);
               WRITE_PORT_UCHAR((UCHAR *)0x3c5, 0 ); // disable SVGA 
 
Получить адрес начала видеопамяти так:
                WRITE_PORT_UCHAR((UCHAR *)0x3c4, 0xa2);
                pLFB = (VOID *)READ_PORT_ULONG((UCHAR *)0x3c5);

*/



#define PARALLELS_VIDEO_DRV_DEFAULT_X_SIZE 1024
#define PARALLELS_VIDEO_DRV_DEFAULT_Y_SIZE 768

/*
void parallels_draw_mouse_bp2(void);
void parallels_set_mouse_cursor_bp2( drv_video_bitmap_t *cursor );
void parallels_mouse_on_bp2(void);
void parallels_mouse_off_bp2(void);
*/

static int parallels_video_probe();
static int parallels_video_start();
static int parallels_video_stop();
static void parallels_video_update(void);
static errno_t parallels_accel_start(void);

//static void parallels_detect2(int xsize, int ysize, int bpp);


struct drv_video_screen_t        video_driver_parallels_svga =
{
    "Parallels SVGA",
    // size
    PARALLELS_VIDEO_DRV_DEFAULT_X_SIZE, PARALLELS_VIDEO_DRV_DEFAULT_Y_SIZE,
    // bpp
    32,
    // mouse x y flags
    0, 0, 0,

    // screen
.screen		=	0,

.probe      =	parallels_video_probe,
.start		= 	parallels_video_start,
//.accel		=	parallels_accel_start,
.stop		=	parallels_video_stop,

//.update		=	parallels_video_update,

// todo hw mouse!
#if 0
mouse_redraw_cursor: 	drv_video_draw_mouse_deflt,
mouse_set_cursor: 	drv_video_set_mouse_cursor_deflt,
mouse_disable:          drv_video_mouse_off_deflt,
mouse_enable:          	drv_video_mouse_on_deflt,
#endif

};


static int psvga_found = 0;

static int parallels_video_probe()
{
    //return gSVGA.found ? VIDEO_PROBE_SUCCESS : VIDEO_PROBE_FAIL;
    return psvga_found ? VIDEO_PROBE_SUCCESS : VIDEO_PROBE_FAIL;
}





// need 4Mbytes aperture
#define N_PAGES 1024

static void parallels_map_video(int on_off, physaddr_t vmem )
{

    if( video_driver_parallels_svga.screen == 0 )
    {
        void *vva;

        if( hal_alloc_vaddress(&vva, N_PAGES) )
            panic("Can't alloc vaddress for %d videmem pages", N_PAGES);

        video_driver_parallels_svga.screen = vva;
    }

    hal_pages_control( vmem, video_driver_parallels_svga.screen, N_PAGES, on_off ? page_map_io : page_unmap, page_rw );

}

static int parallels_video_start()
{

    switch_screen_bitblt_to_32bpp(1);

    outb(0x3c4, 0xa9);
    outb(0x3c5, 32 ); // bpp - 8/15/16/24/32 
    outw(0x3c5, PARALLELS_VIDEO_DRV_DEFAULT_X_SIZE);
    outw(0x3c5, PARALLELS_VIDEO_DRV_DEFAULT_Y_SIZE);
    outw(0x3c5, PARALLELS_VIDEO_DRV_DEFAULT_X_SIZE*4 ); // bytes per line
    outw(0x3c5, 60 ); // refresh rate - value is ignored 
    outb(0x3c5, 1 ); // enable SVGA
    outl(0x3c5, 0 ); //  framebuffer offset 


    outb( 0x3c4, 0xa2 );
    physaddr_t vmem = inl(0x3c5);

    SHOW_FLOW( 0, "video mem phys %lld", (long long) vmem );

    parallels_map_video( 1, vmem );

    return 0;
}

static int parallels_video_stop()
{
    parallels_map_video(0, 0);

    outb( 0x3c4, 0xae);
    outb( 0x3c5, 0 ); // disable SVGA 

    video_drv_basic_vga_set_text_mode();
    return 0;
}



// -----------------------------------------------------------------------
// Parallels SVGA PCI init
// -----------------------------------------------------------------------

phantom_device_t * driver_parallels_svga_pci_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;
    SHOW_FLOW0( 0, "Init" );

    psvga_found = 1;

    phantom_pci_enable( pci, 1 );

    SHOW_FLOW( 1, "pci base 0-3 %p, %p, %p", (void *)pci->base[0], (void *)pci->base[1], (void *)pci->base[2] );


    // We have to report success or there will be more attempts to probe

    phantom_device_t * dev = calloc(1, sizeof(phantom_device_t));
    assert(dev != 0);

    dev->name = "ParallelsSVGA";
    dev->seq_number = 0;

    return dev;

}



#endif // ARCH_ia32
