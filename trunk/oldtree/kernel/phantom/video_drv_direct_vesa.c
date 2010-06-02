#include "hal.h"
#include "video.h"

#include <i386/pio.h>
#include <phantom_libc.h>

#include <kernel/vm.h>


static int direct_vesa_probe();

struct drv_video_screen_t        video_driver_direct_vesa =
{
    "Protected mode Vesa",
    // size
    0, 0,
    // mouse x y flags
    0, 0, 0,

    // screen
    0,

probe: 	direct_vesa_probe,
start: 	(void *)drv_video_null,
stop:  	(void *)drv_video_null,

update:	drv_video_null,
bitblt: (void *)drv_video_null,
winblt: (void *)drv_video_null,
readblt: (void *)drv_video_null,

mouse:  drv_video_null,
redraw_mouse_cursor: 	drv_video_draw_mouse_deflt,
set_mouse_cursor: 	drv_video_set_mouse_cursor_deflt,
mouse_disable:          drv_video_mouse_off_deflt,
mouse_enable:          	drv_video_mouse_on_deflt,
};



static int direct_vesa_probe()
{
    const int ROM_pa = 0xC0000;
    const int ROM_size = 32*1024;
    const int ROM_pages = ROM_size/4096;

    char *ROM_va = (char *)phystokv(ROM_pa);

    hal_pages_control( ROM_pa, ROM_va, ROM_pages, page_map, page_ro );

    char *p = ROM_va;
    printf("Look for VESA PM entry starting at 0x%X\n", p);

    char *entry = 0;
    int cnt = ROM_size;
    while(cnt--)
    {
        if(
           p[0] == 'P' && p[1] == 'M' &&
           p[2] == 'I' && p[3] == 'D' )
        {
            printf("Found VESA PM entry at 0x%X\n", p);
            entry = p;
            break;
        }
        p++;
    }

    if(p == NULL)
        printf("no VESA PM entry");

    hal_pages_control( ROM_pa, ROM_va, ROM_pages, page_unmap, page_ro );

    // Move to another driver
#if 0
    printf("Calling real mode interrupt");

    /*
  
    do_16bit( KERNEL_CS, KERNEL_16_CS,
              //i16_puts("In 16 bit mode");
            );

    */


	int err;
	struct trap_state ts;

	init_ts(&ts);

	ts.trapno = 0x10;
	ts.eax = 0x4F00;
        ts.v86_es = 0x60;
        ts.edi = 0;

	base_real_int(&ts);
	//if ((err = dos_check_err(&ts)) != 0)		return err;
	//if (!(ts.edx & (1<<7)))		return ENOTTY;

    printf("End of real mode interrupt");
#endif

    // Experimental code
    return 0;
}













