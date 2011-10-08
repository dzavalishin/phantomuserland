#include<stdlib.h>
#include<stdio.h>
//#include<jpeglib.h>
//#include<menuet/os.h>
//#include"libmgfx.h"

#define SPLASH_FILE 	"splash.jpg"

static int splash_thread_pid;
static char * splash_thread_stk;
//static mgfx_image_t * splash=NULL;
static int xres,yres,bpp,bpscan;

void I_EndSplash(void);

static void repaint_splash_wnd(void)
{
#if 0
 __menuet__window_redraw(1);
 __menuet__define_window(
     (xres-splash->width)>>1,
     (yres-splash->height)>>1,
     splash->width,
     splash->height+20,
     0x01000000,
     0x00000000,
     0x00000000);
 paint_image(0,0,splash);
 __menuet__window_redraw(2);
#endif
}

static int __tmp=0;

static void splash_thread(void)
{
#if 0
 __menuet__dga_get_caps(&xres,&yres,&bpp,&bpscan);
 repaint_splash_wnd();
 for(;;)
 {
  __menuet__delay100(5);
  __menuet__bar(0,splash->height+1,splash->width,20,0);
  __menuet__bar(0,splash->height+1,__tmp,20,0x404040);
  __tmp+=4;
  if(__tmp>=splash->width) __tmp=0;
 }
#endif
}

void I_BeginSplash(void)
{
#if 0
 init_mgfx_library();
 if(load_image(SPLASH_FILE,&splash)!=_PIC_OK)
 {
  splash=NULL;
  __libclog_printf("Unable to open %s for splash screen !!!\n",SPLASH_FILE);
  return;
 }
 splash_thread_stk=__menuet__exec_thread(splash_thread,4096,&splash_thread_pid);
 if(splash_thread_pid<1)
 {
  __libclog_printf("Unable to create splash screen thread\n");
//  I_EndSplash();
  return;
 }
 __libclog_printf("Splash screen created\n");
#endif
}

void I_EndSplash(void)
{
#if 0
 int i;
 if(!splash) return;
 if(splash_thread_pid>1)
  __asm__ __volatile__("int $0x40"::"a"(18),"b"(2),"c"(splash_thread_pid));
 free_image(splash);
 free(splash);
 splash=NULL;
 free(splash_thread_stk);
#endif
}
