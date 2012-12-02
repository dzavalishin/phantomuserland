#ifdef ARCH_arm

/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2013 Dmitry Zavalishin, dz@dz.ru
 *
 * ARM raspberry PI (bcm2835) framebuffer.
 *
**/



#define DEBUG_MSG_PREFIX "bcm2835framebuffer"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <arm/memio.h>

#include <kernel/page.h>
#include <kernel/debug.h>
#include <kernel/barriers.h>

#include <sys/types.h>

#include <phantom_assert.h>
#include <errno.h>
#include <stdio.h>

#include <video/screen.h>
#include <video/internal.h>


#include "driver_arm_raspberry_fb.h"

#define FB_DEFAULT_W 800
#define FB_DEFAULT_H 600
#define FB_DEFAULT_BPP 24

#define PERIPHERAL_BASE 0x20000000 /* Base address for all (ARM?) peripherals */

static physaddr_t       video_fb_pa = 0;
static void *           video_fb_va = 0;
static size_t           video_fb_bytes = 0;



static u_int32_t ArmToVc(void *p)
{
	//Some things (e.g: the GPU) expect bus addresses, not ARM physical
	//addresses
	return ((u_int32_t)p) + 0xC0000000;
}

/*
static void *VcToArm(u_int32_t p)
{
	//Go the other way to ArmToVc
	return (void *)(p - 0xC0000000);
}
*/


static u_int32_t mbox_read()
{
    u_int32_t r = 0;

    do {
        mem_barrier();
        while (R32(MAIL_BASE+MAIL_STATUS+PERIPHERAL_BASE) & MAIL_EMPTY)
            mem_barrier(); // wait for data

        r = R32(MAIL_BASE+MAIL_READ+PERIPHERAL_BASE); // Read the data
    } while ((r & 0xF) != MAIL_FB);
    //Loop until we received something from the frame buffer channel

    return r & 0xFFFFFFF0;
}

static void mbox_write(u_int32_t v)
{
    mem_barrier();
    while(R32(MAIL_BASE+MAIL_STATUS+PERIPHERAL_BASE) & MAIL_FULL)
        mem_barrier(); // wait for space

    //Write the value to the frame buffer channel
    W32(MAIL_BASE+MAIL_WRITE+PERIPHERAL_BASE, MAIL_FB | (v & 0xFFFFFFF0));
    mem_write_barrier();
}


volatile struct Bcm2835FrameBuffer __attribute__ ((aligned (16))) fb_buf;


static errno_t try_init_fb(void)
{
	//Some (or even all?) of these memory barriers can probably be omitted safely
	//MemoryBarrier();

    volatile struct Bcm2835FrameBuffer *fb = &fb_buf;
    assert( ( ((addr_t)fb) & 0xF) == 0 );

    fb->width = FB_DEFAULT_W;
    fb->height = FB_DEFAULT_H;
    fb->vwidth = fb->width;
    fb->vheight = fb->height;
    fb->pitch = 0;
    fb->depth = FB_DEFAULT_BPP;
    fb->x = 0;
    fb->y = 0;
    fb->pointer = 0;
    fb->size = 0;

    mem_write_barrier();

    mbox_write(ArmToVc((void *)fb)); //Tell the GPU the address of the structure

    u_int32_t r = mbox_read(); //Wait for the GPU to respond, and get its response

    if (r)
    {
        //If r != 0, some kind of error occured
        //WriteGpio(17);
        return ENXIO;
    }

    if (!fb->pointer)
    {
        //if the frame buffer pointer is zero, an error occured
        //WriteGpio(18);
        return ENOMEM;
    }

    video_fb_pa = fb->pointer;
    video_fb_bytes = fb->width * fb->height * (fb->depth / 8);

    return 0;
}






void arm_raspberry_video_init(void)
{
    // Can't live without you?
    while( try_init_fb() )
        ;


}




static int raspberry_pi_video_probe();
static int raspberry_pi_video_start();
static int raspberry_pi_video_stop();



struct drv_video_screen_t        video_driver_raspberry_pi =
{
    "Raspberry PI",

    FB_DEFAULT_W, FB_DEFAULT_H, FB_DEFAULT_BPP,
    // mouse x y flags
    0, 0, 0,

    // screen
screen:			0,

probe: 			raspberry_pi_video_probe,
start: 			raspberry_pi_video_start,
stop:   		raspberry_pi_video_stop,

mouse_redraw_cursor: 	(void *)vid_null,
mouse_set_cursor: 	(void *)vid_null,
mouse_disable:          (void *)vid_null,
mouse_enable:          	(void *)vid_null,

};



static int raspberry_pi_video_probe()
{
    if( video_fb_pa == 0 )
        return VIDEO_PROBE_FAIL;

    if(video_fb_va == 0)
    {
        //hal_pv_alloc( &video_fb_pa, &video_fb_va, VRAM_SIZE );
        errno_t rc = hal_alloc_vaddress( &video_fb_va, BYTES_TO_PAGES(video_fb_bytes) );
        if( rc )
        {
            SHOW_ERROR( 0, "VA alloc failed (%d)", rc );
            return VIDEO_PROBE_FAIL;
        }
    }

    // TODO switch to virtual on paging start?
    video_driver_raspberry_pi.screen = video_fb_va;

    SHOW_FLOW( 7, "vmem va 0x%X pa 0x%X", video_driver_raspberry_pi.screen, video_fb_pa);
    SHOW_INFO( 0, "Raspberry PI video %d*%d found",  video_driver_raspberry_pi.xsize, video_driver_raspberry_pi.ysize );
    return VIDEO_PROBE_SUCCESS;
}






static void raspberry_pi_map_video(int on_off)
{
    (void) on_off;

    assert( video_driver_raspberry_pi.screen != 0 );

    // TODO uncached?
    hal_pages_control_etc( video_fb_pa, video_driver_raspberry_pi.screen,
                           BYTES_TO_PAGES(video_fb_bytes),
                           on_off ? page_map_io : page_unmap, page_rw, 0 );

}



static int raspberry_pi_video_start()
{
    switch_screen_bitblt_to_32bpp(1);
    raspberry_pi_map_video( 1 );
    return 0;
}

static int raspberry_pi_video_stop()
{
    raspberry_pi_map_video(0);
    return 0;
}


























#endif // ARCH_arm



