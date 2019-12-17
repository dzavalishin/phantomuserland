/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Intel i810 video card driver. Incomplete.
 *
 * This driver is supposed to be addition to VESA mode select and videomem access
 * driver. This driver will supply hardware cursor only.
 * 
 * Hopefully it will support wide range of Intel video hardware.
 * 
 * Init sequence:
 * 
 *   * driver_intel_810_pci_probe() is called first from pci drivers lookup code.
 *   * i810_video_probe() is called next from video engine startup code. 
 *   * i810_video_start() is called if two funcs above reported success.
 * 
**/

#if defined(ARCH_ia32) || 1

#define DEBUG_MSG_PREFIX "video-intel"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <hal.h>
#include <assert.h>
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

static int i810_video_probe();
static int i810_video_start();
static int i810_video_stop();

static int i810_init();

static void prepareHardwareCursor( void );
static void setCursorEnable( int on );

void scr_convert_cursor_to_2bpp( void *dst, drv_video_bitmap_t *cursor, int dst_xsize, int dst_ysize );


struct drv_video_screen_t        video_driver_i810 =
{
    "Intel810",
    // size
    800, 600, 32,
    // mouse x y flags
    0, 0, 0,

    // screen
screen:			0,

.probe = 			i810_video_probe,
//start: 			i810_video_start,
.accel = 			i810_video_start,
.stop =   		i810_video_stop,

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


typedef struct {
    int         dummy;
    void        *mmio; // Pointer to mapped registers

    physaddr_t  cursor_pa;
    void        *cursor_va;
} i810;


static i810 vcard_s;

static phantom_device_t * dev;
static i810 * vcard = &vcard_s;


#define IGPU_WRITE_REG( __reg, __val ) (  *((u_int32_t *)(vcard->mmio + (__reg))) = __val )
#define IGPU_READ_REG( __reg ) ( (u_int32_t)  *((u_int32_t *)(vcard->mmio + (__reg))) )
#define IGPU_REG( __reg ) (  *((u_int32_t *)(vcard->mmio + (__reg))) )



//static int n_pages = 1024;


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

    //vcard = calloc(1, sizeof(i810));
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
            //dev->iomem = pci->base[i];
            //dev->iomemsize = pci->size[i];

            //n_pages = dev->iomemsize/PAGE_SIZE; // FIXME partial page

            SHOW_INFO( 1,  "%d: mem base 0x%x, size %d", i, dev->iomem, dev->iomemsize );
        } else if( pci->base[i] > 0) {
            dev->iobase = pci->base[i];
            SHOW_INFO( 1,  "%d: io_port base 0x%x, size %d", i, dev->iobase, pci->size[i] );
        }
    }

    physaddr_t mmadr = pci->base[0];

    SHOW_INFO( 0,  "mmio base 0x%x, size %d", mmadr, pci->size[0] );

    if( mmadr == 0 )
    {
        SHOW_ERROR( 0,  "mmio base 0x%x, fail", mmadr );
        goto free;
    }

    int n_pages = BYTES_TO_PAGES(pci->size[0]);

    if( hal_alloc_vaddress(&vcard->mmio, n_pages) )
        panic("Can't alloc vaddress for %d mmio pages", n_pages);

    hal_pages_control_etc( mmadr, vcard->mmio, n_pages, 
                            page_map_io, page_rw,
                            INTEL_PTE_WTHRU|INTEL_PTE_NCACHE );

    SHOW_FLOW( 7, "mmio va 0x%X", vcard->mmio );


    SHOW_FLOW0( 1, "init");
    if (i810_init() < 0)
    {
        SHOW_ERROR0( 0, "init failed");
        goto free;
    }


    dev->name = "i810";
    dev->seq_number = seq_number++;
    dev->drv_private = vcard;

    phantom_pci_enable( pci, 1 );

    return dev;

free:
    //free(vcard);
    free(dev);
    return 0;
}






static int i810_video_probe()
{
    SHOW_FLOW0( 1, "Probe" );
    if(!seq_number)
    {
        SHOW_ERROR( 0, "seq = %d", seq_number );
        return VIDEO_PROBE_FAIL;
    }


    //printf("Intel board 0x%x found\n", id);
    SHOW_FLOW0( 1, "Success" );
    //return VIDEO_PROBE_SUCCESS;
    return VIDEO_PROBE_ACCEL;
}



static void i810_set_cursor_pos( void );
static void i810_set_mouse_cursor( drv_video_bitmap_t *cursor );

static int i810_video_start()
{
    prepareHardwareCursor();
    video_drv->mouse_redraw_cursor = i810_set_cursor_pos;
    video_drv->mouse_set_cursor = i810_set_mouse_cursor;

    // Turn off unused funcs
    video_drv->mouse_disable = vid_null;
    video_drv->mouse_enable = vid_null;
    SHOW_FLOW0( 1, "Started" );
    return 0;
}

static int i810_video_stop()
{
    setCursorEnable( 0 );
    return 0;
}





#define IGPU_CURACNTR                   0x70080
#define IGPU_CURBCNTR 0x700C0

#define IGPU_CURxCNTR_POPUP             (1<<27)
//#define IGPU_CURACNTR_MODE_ARGB        (1<<5)
#define IGPU_CURxCNTR_MODE_64x64_XOR    0b101

// A write to this register also acts as a trigger event to force the update 
// of active registers from the staging registers on the next display event.
#define IGPU_CURABASE                        0x70084
#define IGPU_CURAPOS                         0x70088

static void setDefaultCursorMode( void )
{
    u_int32_t v = IGPU_CURxCNTR_POPUP | IGPU_CURxCNTR_MODE_64x64_XOR;
    IGPU_WRITE_REG( IGPU_CURACNTR, v );
}

static physaddr_t last_cura_physmem;

static void setCursorBitmapAddress( physaddr_t cbmp )
{
    //assert( 0 == (cbmp & 0x7) );

    // Must be 4K aligned    
    assert( 0 == (cbmp & 0xfff) );

    // TODO 64 bit not ready, low 3 bits are bits [35:32] of address
    last_cura_physmem = cbmp;
    IGPU_WRITE_REG( IGPU_CURABASE, cbmp );
}

static void setCursorPosition( int x, int y )
{
    u_int32_t v = 0;

    x &= 0b111111111111;
    y &= 0b111111111111;

    // Topmost bits of halves are signs
    // We assume them to be zero
    v |= x;
    v |= ((u_int32_t)y) << 16;

    IGPU_WRITE_REG( IGPU_CURAPOS, v );
    IGPU_WRITE_REG( IGPU_CURABASE, last_cura_physmem ); // Commit
}



#define IGPU_CURAPALET0 0x70090
#define IGPU_CURAPALET1 0x70094
#define IGPU_CURAPALET2 0x70098
#define IGPU_CURAPALET3 0x7009C

static void setCursorPalette( u_int32_t p[4] )
{
    IGPU_WRITE_REG( IGPU_CURAPALET0, p[0] );
    IGPU_WRITE_REG( IGPU_CURAPALET1, p[1] );
    IGPU_WRITE_REG( IGPU_CURAPALET2, p[2] );
    IGPU_WRITE_REG( IGPU_CURAPALET3, p[3] );
    IGPU_WRITE_REG( IGPU_CURABASE, last_cura_physmem ); // Commit
}

static void setDefaultCursorPalette( void )
{
    static u_int32_t p[4] = { 0, 0xFFFFFF, 0xFF0FFF, 0xFFFF0F };
    setCursorPalette( p );
}



#define IGPU_PIPEACONF                  0x70008
#define IGPU_PIPEACONF_CURSOR_OFF       (1<<18)

#define IGPU_VGACNTRL                   0x71400
#define IGPU_VGACNTRL_VGA_DISABLE       (1<<31)

static void setCursorEnable( int on )
{
    u_int32_t vc = IGPU_READ_REG( IGPU_VGACNTRL );
    u_int32_t pa = IGPU_READ_REG( IGPU_PIPEACONF );
    LOG_FLOW( 1, "PIPEACONF 0x%x, VGACNTRL 0x%x", pa, vc );

    if( on ) pa &= ~IGPU_PIPEACONF_CURSOR_OFF;
    else     pa |= IGPU_PIPEACONF_CURSOR_OFF;

    IGPU_WRITE_REG( IGPU_PIPEACONF, pa );
    //IGPU_WRITE_REG( IGPU_VGACNTRL, vc );    
}

#define CURSOR_BMP_X_SIZE 64
#define CURSOR_BMP_Y_SIZE 64
#define CURSOR_BMP_BITS   2
#define CURSOR_BMP_MEM_BYTES ((CURSOR_BMP_X_SIZE*CURSOR_BMP_Y_SIZE*CURSOR_BMP_BITS)/8)

static void prepareHardwareCursor( void )
{
    // allocate cursor memory
    hal_pv_alloc_io( &vcard->cursor_pa, &vcard->cursor_va, CURSOR_BMP_MEM_BYTES );
    memset( vcard->cursor_va, 0xFF, CURSOR_BMP_MEM_BYTES );

    setDefaultCursorMode();
    setDefaultCursorPalette();

    setCursorPosition( 10, 10 );
    setCursorBitmapAddress( vcard->cursor_pa );

    setCursorEnable( 1 );
}


// This one is called from plain (non-video) diver entry point
static int i810_init()
{
    u_int32_t vc = IGPU_READ_REG( IGPU_VGACNTRL );
    u_int32_t pa = IGPU_READ_REG( IGPU_PIPEACONF );
    LOG_FLOW( 1, "PIPEACONF 0x%x, VGACNTRL 0x%x", pa, vc );

    return 0;
}



// Called from mouse code on pos update
static void i810_set_cursor_pos( void )
{
    if( 0 == video_drv ) return;
    setCursorPosition( video_drv->mouse_x, video_drv->ysize - video_drv->mouse_y - 1 );
}

static void i810_set_mouse_cursor( drv_video_bitmap_t *cursor )
{
    if( 0 == video_drv ) return;
    if( 0 == vcard->cursor_va ) return;

    //scr_convert_cursor_to_2bpp( vcard->cursor_va, cursor, 64, 64 );
}



void scr_convert_cursor_to_2bpp( void *dst, drv_video_bitmap_t *cursor, int dst_xsize, int dst_ysize )
{
    u_int32_t *p = dst;

    int i, j;

    const int shift_up = dst_ysize - cursor->ysize;
    const int shift_left = dst_xsize - cursor->xsize;

    rgba_t get_cp( int x, int y )
    {
        y = dst_ysize - y - 1;

        y -= shift_up;
        x -= shift_left;

        if( x < 0 ) return (rgba_t) { 0, 0, 0, 0 };
        if( y < 0 ) return (rgba_t) { 0, 0, 0, 0 };
        if( x >= cursor->xsize ) return (rgba_t) { 0, 0, 0, 0 };
        if( y >= cursor->ysize ) return (rgba_t) { 0, 0, 0, 0 };

        return cursor->pixel[ (y*cursor->xsize) + x ];
    }


    for( i = 0; i < dst_ysize; i++ )
    {
        u_int64_t andMask = 0;
        u_int64_t xorMask = 0;


        for ( j = 0; j < dst_xsize; j++ )
        {
            rgba_t pixel = get_cp( dst_xsize - j - 1, i );
            if( pixel.a )
            {
                if( pixel.r || pixel.g || pixel.b )
                {
                    // temp use inverted here, need better cursor
                    //andMask|= 1 << j;  //foreground
                    //xorMask|= 1 << j;

                    andMask|= 1 << j;    //inverted
                }
                else
                    xorMask|= 1 << j;    //background
            }

        }


        p[0]  = andMask;
        p[32] = xorMask;
        p++;
    }

}


#endif // ARCH_ia32


