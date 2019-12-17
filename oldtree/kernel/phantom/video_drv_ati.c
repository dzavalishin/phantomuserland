/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Intel ati video card driver. Incomplete.
 *
 * This driver is supposed to be addition to VESA mode select and videomem access
 * driver. This driver will supply hardware cursor only.
 * 
 * Hopefully it will support wide range of Intel video hardware.
 * 
 * Init sequence:
 * 
 *   * driver_ati_video_pci_probe() is called first from pci drivers lookup code.
 *   * ati_video_probe() is called next from video engine startup code. 
 *   * ati_video_start() is called if two funcs above reported success.
 * 
**/

#if defined(ARCH_ia32) || 1

#define DEBUG_MSG_PREFIX "video-ati"
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



//#define outw( __subreg, __val ) outl( vcard->reg_base + (__subreg), (__val) )
//#define outw( __subreg, __val ) *((u_int32_t)(vcard->mmio + (__subreg << 2))) = (__val)
#define outw( __subreg, __val ) *((u_int32_t *)(vcard->mmio + (__subreg))) = (__val)

static int ati_video_probe();
static int ati_video_start();
static int ati_video_stop();

static int ati_init();


struct drv_video_screen_t        video_driver_ati =
{
    "ATI",
    // size
//    800, 600, 32,
    800, 600, 24, // TODO check for 32bpp mode?
    // mouse x y flags
    0, 0, 0,

    // screen
screen:			0,

.probe =            ati_video_probe,
//.start =            ati_video_start,
.accel =            ati_video_start,
.stop =             ati_video_stop,

};







typedef struct {
    void        *mmio; // Pointer to mapped registers
    void        *vmem; // Pointer to video memory

    //physaddr_t  cursor_pa;
    //void        *cursor_va;

    //u_int16_t   reg_base; // TODO registers base IO address
} ati;


static ati vcard_s;

static phantom_device_t * dev;
static ati * vcard = &vcard_s;


#define IGPU_WRITE_REG( __reg, __val ) (  *((u_int32_t *)(vcard->mmio + (__reg))) = __val )
#define IGPU_READ_REG( __reg ) ( (u_int32_t)  *((u_int32_t *)(vcard->mmio + (__reg))) )
#define IGPU_REG( __reg ) (  *((u_int32_t *)(vcard->mmio + (__reg))) )



//static int n_pages = 1024;


static int seq_number = 0;
phantom_device_t * driver_ati_video_pci_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    if( seq_number )
    {
        SHOW_ERROR0( 0, "Just one");
        return 0;
    }

    SHOW_FLOW0( 1, "probe" );

    //vcard = calloc(1, sizeof(ati));
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
            //vcard->reg_base = pci->base[i];
            SHOW_INFO( 1,  "%d: io_port base 0x%x, size %d", i, dev->iobase, pci->size[i] );
        }
    }


    physaddr_t vmem_adr = pci->base[0];
    size_t vmem_size = pci->size[0];

    SHOW_INFO( 0,  "vmem base 0x%x, size %d", vmem_adr, vmem_size );

    if( vmem_adr == 0 )
    {
        SHOW_ERROR( 0,  "vmem base 0x%x, fail", vmem_adr );
        goto free;
    }

    int n_vm_pages = BYTES_TO_PAGES(vmem_size);

    if( hal_alloc_vaddress(&vcard->vmem, n_vm_pages) )
        panic("Can't alloc vaddress for %d vmem pages", n_vm_pages);

    hal_pages_control_etc( vmem_adr, vcard->vmem, n_vm_pages, 
                            page_map_io, page_rw,
                            INTEL_PTE_WTHRU|INTEL_PTE_NCACHE );

    SHOW_FLOW( 7, "vmem va 0x%X", vcard->vmem );




    physaddr_t mmadr = pci->base[2];
    size_t mmsize = pci->size[2];

    SHOW_INFO( 0,  "mmio base 0x%x, size %d", mmadr, mmsize );

    if( mmadr == 0 )
    {
        SHOW_ERROR( 0,  "mmio base 0x%x, fail", mmadr );
        goto free;
    }

    int n_mm_pages = BYTES_TO_PAGES(mmsize);

    if( hal_alloc_vaddress(&vcard->mmio, n_mm_pages) )
        panic("Can't alloc vaddress for %d mmio pages", n_mm_pages);

    hal_pages_control_etc( mmadr, vcard->mmio, n_mm_pages, 
                            page_map_io, page_rw,
                            INTEL_PTE_WTHRU|INTEL_PTE_NCACHE );

    SHOW_FLOW( 7, "mmio va 0x%X", vcard->mmio );



    SHOW_FLOW0( 1, "init");
    if (ati_init() < 0)
    {
        SHOW_ERROR0( 0, "init failed");
        goto free;
    }


    dev->name = "ati";
    dev->seq_number = seq_number++;
    dev->drv_private = vcard;

    phantom_pci_enable( pci, 1 );

    return dev;

free:
    //free(vcard);
    free(dev);
    return 0;
}






static int ati_video_probe()
{
#if 1
    return VIDEO_PROBE_FAIL;
#else
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
#endif
}



static void ati_set_cursor_pos( void );
static void ati_set_mouse_cursor( drv_video_bitmap_t *cursor );

static int ati_video_start()
{
#if 0    
    prepareHardwareCursor();
    video_drv->mouse_redraw_cursor = ati_set_cursor_pos;
    video_drv->mouse_set_cursor = ati_set_mouse_cursor;

    // Turn off unused funcs
    video_drv->mouse_disable = vid_null;
    video_drv->mouse_enable = vid_null;
    SHOW_FLOW0( 1, "Started" );
    return 0;
#endif
}

static int ati_video_stop()
{
    //setCursorEnable( 0 );
    return 0;
}



#define D1CUR_CONTROL         0x6400
#define D1CURSOR_EN                      (1<<0)
#define D1CURSOR_MODE_2_MONO             (0b00<<8)
#define D1CURSOR_MODE_32_PREMULT_ALPHA   (0b10<<8)
#define D1CURSOR_MODE_32_UNMULT_ALPHA    (0b11<<8)
#define D1CURSOR_2X_MAGNIFY              (1<<16)

#define D1CUR_SURFACE_ADDRESS 0x6408

#define D1CUR_SIZE            0x6410

#define D1CUR_POSITION        0x6414

#define D1CUR_HOT_SPOT        0x6418

#define D1CUR_UPDATE          0x6424
#define D1CURSOR_UPDATE_PENDING           (1<<0)
#define D1CURSOR_UPDATE_TAKEN             (1<<1)
#define D1CURSOR_UPDATE_LOCK              (1<<16)


static void ati_setup_cursor( drv_video_bitmap_t *cursor )
{
    // suppose we have no more than 1280*2*1024* 32 bpp - that's ten megabytes
    off_t cursor_image_offset = 1280*2*1024 * 4;
    // now align up to 4K bytes 
    
    cursor_image_offset &= ~0b11111111111;
    cursor_image_offset += 0b100000000000; // and one page up

    // now upload cursor to video mem
    // TODO check that we are inside aperture - we'll need 16 mb aperture

    size_t cursor_bytes = cursor->xsize * cursor->ysize * 4;
    memcpy( vcard->vmem + cursor_image_offset, cursor->pixel, cursor_bytes );

    outw( D1CUR_UPDATE, 0 );

    outw( D1CUR_SURFACE_ADDRESS, cursor_image_offset );

    outw( D1CUR_SIZE, ((cursor->xsize & 0x1F)  << 16) | (cursor->ysize & 0x1F) );

    outw( D1CUR_HOT_SPOT, 0 );
    outw( D1CUR_POSITION, 0 );

    outw( D1CUR_CONTROL, D1CURSOR_MODE_32_UNMULT_ALPHA|D1CURSOR_EN );
}


// Called from mouse code on pos update
static void ati_set_cursor_pos( void )
{
    if( 0 == video_drv ) return;
    int x = video_drv->mouse_x;
    int y = video_drv->ysize - video_drv->mouse_y - 1;
    outw( D1CUR_POSITION, ((x & 0xFFF)  << 16) | (y & 0xFFF) );
}


#if 0


//-------------------------------------------------------------
// EnableHWCursor - turn on the hardware cursor
void EnableHWCursor(void)
{
    iow16(GEN_TEST_CNTL, GEN_TEST_CNTL_0,
    ior16(GEN_TEST_CNTL, GEN_TEST_CNTL_0) | 0x80);
}
// -----------------------------------------------------------
// DisableHWCursor - turn off the hardware cursor
void DisableHWCursor(void)
{
    iow16(GEN_TEST_CNTL, GET_TEST_CNTL_0,
    ior16(GEN_TEST_CNTL, GEN_TEST_CNTL_0) & (~0x80L));
}

//-------------------------------------------------------------
// SetHWCursorPos - set the hardware cursor position relative to hotspot
// It is assumed that the cursor has been previously defined
// linearly in off-screen memory with a pitch of 64 pixels (16
// bytes, or 2 QWORDs).
// CUR_OFFSET = QWORD offset of cursor definition in graphics memory
// CUR_HORZ_OFF = 64 - cursorWidth
// CUR_VERT_OFF = 64 - cursorHeight
void SetHWCursorPos(short x, short y)
{
    unsigned short curHorzOff, curVertOff;
    unsigned curOffset;
    
    static bool prevViolation = 0;
    bool violation = 0;

    curOffset = cur.offset;

    // Check for coordinate violations.
    if ((x - cur.hotSpot.x) < 0) 
    {
        curHorzOff = 64 - cur.width - (x - cur.hotSpot.x);
        x = 0;
        violation = 1;
    } 
    else 
        curHorzOff = 64 - cur.width;

    if ((y - cur.hotSpot.y) < 0) 
    {
        curVertOff = 64 - cur.height - (y - cur.hotSpot.y);
        curOffset = cur.offset + (cur.hotSpot.y - y) * 2;
        y = 0;
        violation = 1;
    } 
    else 
        curVertOff = 64 - cur.height;

    if (violation || prevViolation) 
    {
        regw(CUR_OFFSET, curOffset);
        regw(CUR_HORZ_VERT_OFF, ((u_int32_t)curVertOff << 16) | curHorzOff);
    }

    prevViolation = violation;

    // Set the cursor position.
    regw(CUR_HORZ_VERT_POSN, ((u_int32_t)y << 16) | x);
}

#endif





#endif
