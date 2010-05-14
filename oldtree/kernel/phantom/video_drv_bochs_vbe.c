#include "hal.h"
#include <kernel/vm.h>
#include <x86/phantom_pmap.h>

#include <i386/pio.h>
#include <phantom_libc.h>

#include "video.h"
#include "video_drv_bochs_vbe.h"

static int bochs_video_probe();
static int bochs_video_start();
static int bochs_video_stop();


// TODO - try 32bpp?

struct drv_video_screen_t        video_driver_bochs_vesa_emulator =
{
    "Bochs VBE emulator",
    // size
    BOCHS_VIDEO_DRV_DEFAULT_X_SIZE, BOCHS_VIDEO_DRV_DEFAULT_Y_SIZE,
    // mouse x y flags
    0, 0, 0,

    // screen
screen:			0,

probe: 			bochs_video_probe,
start: 			bochs_video_start,
stop:   		bochs_video_stop,

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

static int n_pages = 1024;
static int bochs_video_probe()
{
    //printf("Probing for ");
    vbe_write(VBE_DISPI_INDEX_ID, VBE_DISPI_ID2);
    int id = vbe_read(VBE_DISPI_INDEX_ID);

    if( id < VBE_DISPI_ID2 || id > VBE_DISPI_ID3 )
        return 0;

    if( hal_alloc_vaddress((void **)&video_driver_bochs_vesa_emulator.screen, n_pages) )
        panic("Can't alloc vaddress for %d videmem pages", n_pages);


    printf("Bochs VBE emulator ver 0x%x found\n", id);
    return 1;
}




#define VBE_DISPI_LFB_PHYSICAL_ADDRESS  0xE0000000


static void bochs_map_video(int on_off)
{
#if 1
    assert( video_driver_bios_vesa.screen != 0 );

    hal_pages_control_etc(
                          VBE_DISPI_LFB_PHYSICAL_ADDRESS,
                          video_driver_bochs_vesa_emulator.screen,
                          n_pages, page_map, page_rw,
                          INTEL_PTE_WTHRU|INTEL_PTE_NCACHE );

#else
    int i;
    // TODO add to HAL map function to process more than 1 page
    // need 4Mbytes aperture
    for( i = 0; i < 1024; i++ )
    {
        int pa = VBE_DISPI_LFB_PHYSICAL_ADDRESS + (i << 12);
        char *p = (char *)phystokv( pa );
        hal_page_control( pa, p, on_off ? page_map : page_unmap, page_rw );
    }

    video_driver_bochs_vesa_emulator.screen = (void *)phystokv( VBE_DISPI_LFB_PHYSICAL_ADDRESS );
#endif
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

#if 0
void test_vesa_emu()
{
    printf("\n\nEntering graphics mode, press enter...");
    getchar();

#if 0
    //vid_VesaDriver_SetVideoMode(1024,768,24);
    if( vbe_set(BOCHS_VIDEO_DRV_DEFAULT_X_SIZE, BOCHS_VIDEO_DRV_DEFAULT_Y_SIZE, 24) )
    {
        printf("No VBE, can't set graph mode\n");
        return;
    }

    /*
    char *p = phystokv(VBE_DISPI_LFB_PHYSICAL_ADDRESS);
    //char *p = VBE_DISPI_LFB_PHYSICAL_ADDRESS;

    // TODO need 4Mbytes aperture
    hal_page_control( VBE_DISPI_LFB_PHYSICAL_ADDRESS, p, page_map, page_rw );
    */

    bochs_map_video(1);
#endif
    bochs_video_start();

    char *p = video_driver_bochs_vesa_emulator.screen;

    int len = 1000*100;
    while(len--)
        *p++ = 0xFF;


    printf("\n\nLeaving graphics mode, press enter...");
    getchar();

    bochs_video_stop();

}
#endif

