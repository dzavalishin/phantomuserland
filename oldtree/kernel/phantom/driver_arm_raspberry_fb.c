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

#include <kernel/debug.h>
#include <kernel/barriers.h>

#include <sys/types.h>

#include <phantom_assert.h>
#include <errno.h>

#include "driver_arm_raspberry_fb.h"

#define PERIPHERAL_BASE 0x20000000 /* Base address for all (ARM?) peripherals */


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

    fb->width = 640;
    fb->height = 480;
    fb->vwidth = fb->width;
    fb->vheight = fb->height;
    fb->pitch = 0;
    fb->depth = 24;
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

    return 0;
}






void arm_raspberry_video_init(void)
{
    // Can't live without you?
    while( try_init_fb() )
        ;


}






#endif // ARCH_arm



