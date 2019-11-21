/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * S3 Virge video card driver. Incomplete. Supposed to be mouse cursor accelerator only.
 *
 *
**/

//#ifdef ARCH_ia32

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

//#include <video.h>
#include <video/screen.h>
#include <video/internal.h>

static int s3_video_probe();
static int s3_video_start();
static int s3_video_stop();

static int s3_init();



struct drv_video_screen_t        video_driver_s3 =
{
    "S3",
    // size
    800, 600, 32,
    // mouse x y flags
    0, 0, 0,

    // screen
screen:			0,

probe: 			s3_video_probe,
start: 			s3_video_start,
stop:   		s3_video_stop,

#if 0
update: 		vid_null,
bitblt: 		drv_video_bitblt_rev,
winblt:			drv_video_win_winblt_rev,
readblt: 		drv_video_readblt_rev,
bitblt_part:            drv_video_bitblt_part_rev,

mouse:    		vid_null,

redraw_mouse_cursor: 	drv_video_draw_mouse_deflt,
set_mouse_cursor: 	drv_video_set_mouse_cursor_deflt,
mouse_disable:          drv_video_mouse_off_deflt,
mouse_enable:          	drv_video_mouse_on_deflt,
#endif

};




static inline u_int8_t readReg(int IndexPort, u_int8_t RegIndex)
{
    outb(IndexPort, RegIndex);
    return inb(IndexPort + 1);
}

static inline void writeReg(int IndexPort, u_int8_t RegIndex, u_int8_t RegValue)
{
    outb( IndexPort, RegIndex );
    outb( IndexPort+1, RegValue );
}

static inline u_int8_t readCR(u_int8_t RegIndex)
{
    return ReadReg( 0x3D4, RegIndex );
}

static inline void writeCR( u_int8_t RegIndex, u_int8_t RegValue )
{
    writeReg( 0x3D4, RegIndex,  RegValue );
}



static void setCursorColors( color_t bg, color_t fg )
{
    writeCR( 0x39, 0xA0 ); // Unlock Sys Control regs

    u_int8_t CR45 = readCR( 0x45 ); // reset RGB stack access counter
    writeCR( 0x4B, bg.r );
    writeCR( 0x4B, bg.g );
    writeCR( 0x4B, bg.b );

    u_int8_t CR45 = readCR( 0x45 ); // reset RGB stack access counter
    writeCR( 0x4A, fg.r );
    writeCR( 0x4A, fg.g );
    writeCR( 0x4A, fg.b );

    writeCR( 0x45, CR45 | 0x1 ); // Enable cursor

}


static void setCursorPosition( int x, int y )
{
    writeCR( 0x39, 0xA0 ); // Unlock Sys Control regs

    writeCR( 0x46, 0x7 & (x >> 8) ); 
    writeCR( 0x47, 0xFF & x ); 

    writeCR( 0x48, 0x7 & (y >> 8) ); 
    writeCR( 0x49, 0xFF & y ); 

    writeCR( 0x4E, 0 ); // x offset into cursor img
    writeCR( 0x4F, 0 ); // y offset into cursor img
}


size_t getVideoMemSize()
{
    writeCR( 0x39, 0xA0 ); // Unlock Sys Control regs

    u_int8_t CR36 = readCR( 0x36 );
    u_int8_t CR58 = readCR( 0x58 );

    if( (CR58 & 0x10) == 0 )
    {
        LOG_ERROR( 0, "Strange: Linear mode not set, CR58 = 0x%x", CR58 );
        CR58 |= 0x10;
    }

    CR36 >>= 5;

    switch(CR36)
    {
        default:
            LOG_ERROR( 0, "wrong mem size, CR36 = ", readCR( 0x36 ) );
            /* FALLTHROUGH */
        case 0b100: 
            CR58 &= ~0b1; // 2MB aperture
            CR58 |= 0b10;
            writeCR( 0x58, CR58 );
            return 2 * 1024 * 1024;

        case 0b000: 
            CR58 |= 0b11; // 4MB aperture
            writeCR( 0x58, CR58 );
            return 4 * 1024 * 1024;
    }
}


static void setCursorImage( drv_video_bitmap_t *cb )
{
    writeCR( 0x39, 0xA0 ); // Unlock Sys Control regs

    u_int8_t CR55 = readCR( 0x55 );
    writeCR( 0x55, CR55 & ~0x10 ); // Windows HW cursor mode

    size_t ms = getVideoMemSize();

    // Take upper 1Kbyte

    size_t upper1k = (ms/1024) - 1;
    size_t upper1k_shift = upper1k * 1024;

    LOG_INFO_( 1, "cursor 1K segment is at 0x%X, shift is", upper1k, upper1k_shift );

    writeCR( 0x4C, (upper1k >> 8) & 0x0F );
    writeCR( 0x4D, upper1k & 0xFF );

    if( video_driver_s3.screen == 0 )
    {
        LOG_ERROR0( 0 , "No video mem");
    }

    u_int8_t *cursorMem = video_driver_s3.screen + upper1k_shift;

    int i;
    for( i = 0; i < 1024; i ++ )
        cursorMem[i] = 0xFF;
}

typedef struct {
    int         dummy;
} s3;

static phantom_device_t * dev;
static s3 * vcard = NULL;

//static int n_pages = 1024;


static int seq_number = 0;
phantom_device_t * driver_s3_virge_pci_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    if( seq_number )
    {
        SHOW_ERROR0( 0, "Just one");
        return 0;
    }


    SHOW_FLOW0( 1, "probe" );

    vcard = calloc(1, sizeof(s3));
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

            //n_pages = dev->iomemsize/PAGE_SIZE; // FIXME partial page

            SHOW_INFO( 0,  "base 0x%lx, size 0x%lx", dev->iomem, dev->iomemsize );
        } else if( pci->base[i] > 0) {
            dev->iobase = pci->base[i];
            SHOW_INFO( 0,  "io_port 0x%x", dev->iobase );
        }
    }

    SHOW_FLOW0( 1, "init");
    if (s3_init() < 0)
    {
        SHOW_ERROR0( 0, "init failed");
        goto free;
    }


    dev->name = "s3";
    dev->seq_number = seq_number++;
    dev->drv_private = vcard;


    return dev;

free:
    free(vcard);
    free(dev);
    return 0;
}



static int s3_init()
{
    return 0;
}




static int s3_video_probe()
{
    if( (!seq_number) || (vcard == 0) )
        return VIDEO_PROBE_FAIL;
/*
    if( hal_alloc_vaddress((void **)&video_driver_s3.screen, n_pages) )
        panic("Can't alloc vaddress for %d videmem pages", n_pages);

    SHOW_FLOW( 7, "vmem va 0x%X", video_driver_s3.screen);
*/

    //printf("Intel board 0x%x found\n", id);
    //return VIDEO_PROBE_SUCCESS;
    return VIDEO_PROBE_ACCEL;
}





/*
static void s3_map_video(int on_off)
{
    assert( video_driver_s3.screen != 0 );

    hal_pages_control_etc(
                          dev->iomem,
                          video_driver_s3.screen,
                          n_pages, on_off ? page_map : page_unmap, page_rw,
                          INTEL_PTE_WTHRU|INTEL_PTE_NCACHE );
}
*/
static int s3_video_start()
{
    //s3_map_video( 1 );
    return 0;
}

static int s3_video_stop()
{
    //s3_map_video(0);
    
    video_drv_basic_vga_set_text_mode();
    return 0;
}




//#endif // ARCH_ia32




























