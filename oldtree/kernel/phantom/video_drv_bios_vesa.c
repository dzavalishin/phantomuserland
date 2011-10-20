/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Real mode (bios) VESA driver.
 *
 * Most of this driver is in i386/vesa.c
 *
**/


#define DEBUG_MSG_PREFIX "video"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <hal.h>
#include <video/screen.h>
#include <video/internal.h>

#include <ia32/phantom_pmap.h>

#include <phantom_libc.h>
#include <kernel/vm.h>

//static u_int16_t vesaMode = -1;
static int16_t vesaMode = -1;

static void map_video(int on_off);


static int bios_vesa_probe()
{
    // Activated if VESA was found earlier in boot process.
    // See set_video_driver_bios_vesa_mode() below
    return (vesaMode != -1) ? VIDEO_PROBE_SUCCESS : VIDEO_PROBE_FAIL;
}

static int bios_vesa_start()
{
#if !VESA_ENFORCE
    if( setVesaMode(  vesaMode ) )
        return -1;
#endif
    map_video(1);
    return 0;
}

static int bios_vesa_stop()
{
    map_video(0);
//#if !VESA_ENFORCE
#ifdef ARCH_ia32
    setTextVideoMode();
#endif
    // Allways OK
    return 0;
}

struct drv_video_screen_t        video_driver_bios_vesa =
{
    "BIOS Vesa",
    // size
    0, 0, 24,
    // mouse x y flags
    0, 0, 0,

    // screen
    0,

probe: 			bios_vesa_probe,
start: 			bios_vesa_start,
stop:  			bios_vesa_stop,

update: 		drv_video_null,
bitblt: 		drv_video_bitblt_rev,
winblt:			drv_video_win_winblt_rev,
readblt: 		drv_video_readblt_rev,
bitblt_part:            drv_video_bitblt_part_rev,

mouse:  		drv_video_null,

redraw_mouse_cursor: 	drv_video_draw_mouse_deflt,
set_mouse_cursor: 	drv_video_set_mouse_cursor_deflt,
mouse_disable:          drv_video_mouse_off_deflt,
mouse_enable:          	drv_video_mouse_on_deflt,
};


static physaddr_t video_driver_bios_vesa_pa;
static int n_pages;
void set_video_driver_bios_vesa_pa( physaddr_t pa, size_t size )
{
    n_pages = ((size-1)/hal_mem_pagesize())+1;

    video_driver_bios_vesa_pa = pa;

    void *vva;
    if( hal_alloc_vaddress(&vva, n_pages) )
        panic("Can't alloc vaddress for %d vidmem pages", n_pages);

    video_driver_bios_vesa.screen = vva;

    //printf("VESA vaddr = %X, %d pages, %d mbytes\n", vva, n_pages, n_pages*4/1024 );
    SHOW_FLOW( 2, "VESA vaddr = %X, %d pages, %d mbytes", vva, n_pages, n_pages*4/1024 );
}

void set_video_driver_bios_vesa_mode( u_int16_t mode )
{
    vesaMode = mode;
}


static void map_video(int on_off)
{
    assert( video_driver_bios_vesa.screen != 0 );

    hal_pages_control_etc(
                          video_driver_bios_vesa_pa,
                          video_driver_bios_vesa.screen,
                          n_pages, on_off ? page_map : page_unmap, page_rw,
                          INTEL_PTE_WTHRU|INTEL_PTE_NCACHE );


#if 0
    u_int32_t *p = (void *)video_driver_bios_vesa.screen;
    const int rep = 10240;
    int i;

    for( i = rep; i >=0; i-- )
        *p++ = 0xFF0000FF;
    for( i = rep; i >=0; i-- )
        *p++ = 0xFF00FF00;
    for( i = rep; i >=0; i-- )
        *p++ = 0xFFFF0000;
    for( i = rep; i >=0; i-- )
        *p++ = 0xFF000000;


    struct rgba_t buf[rep*4];

    rgba2rgba_move( buf, (void *)video_driver_bios_vesa.screen, rep*4 );
    rgba2rgba_24_move( (void *)p, buf, rep*4 );

    pressEnter("video");
#endif
}











