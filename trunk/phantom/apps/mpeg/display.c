/* display.c, X11 interface                                                 */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#ifdef DISPLAY

 /* the Xlib interface is closely modeled after
  * mpeg_play 2.0 by the Berkeley Plateau Research Group
  */

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "config.h"
#include "global.h"

/* private prototypes */
static void Display_Image _ANSI_ARGS_((XImage *Ximage_Ptr, unsigned char *Dithered_Image));
static void Dither_Frame _ANSI_ARGS_((unsigned char *src[]));
static void Dither_Top_Field _ANSI_ARGS_((unsigned char *src[], unsigned char *dst));
static void Dither_Bottom_Field _ANSI_ARGS_((unsigned char *src[], unsigned char *dst));
static void Dither_Top_Field420 _ANSI_ARGS_((unsigned char *src[],
                                      unsigned char *dst));
static void Dither_Bottom_Field420 _ANSI_ARGS_((unsigned char *src[],
                                      unsigned char *dst));

/* local data */
static unsigned char *Dithered_Image, *Dithered_Image2;

static unsigned char Y_Table[256+16];
static unsigned char Cb_Table[128+16];
static unsigned char Cr_Table[128+16];

/* X11 related variables */
static Display *Display_Ptr;
static Window Window_Instance;
static GC GC_Instance;
static XImage *Ximage_Ptr, *Ximage_Ptr2;
static unsigned char Pixel[256];

#ifdef SH_MEM

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

static int HandleXError _ANSI_ARGS_((Display *dpy, XErrorEvent *event));
static void InstallXErrorHandler _ANSI_ARGS_((void));
static void DeInstallXErrorHandler _ANSI_ARGS_((void));

static int Shmem_Flag;
static XShmSegmentInfo Shminfo1, Shminfo2;
static int gXErrorFlag;
static int CompletionType = -1;

static int HandleXError(Dpy, Event)
Display *Dpy;
XErrorEvent *Event;
{
  gXErrorFlag = 1;

  return 0;
}

static void InstallXErrorHandler()
{
  XSetErrorHandler(HandleXError);
  XFlush(Display_Ptr);
}

static void DeInstallXErrorHandler()
{
  XSetErrorHandler(NULL);
  XFlush(Display_Ptr);
}

#endif

/* connect to server, create and map window,
 * allocate colors and (shared) memory
 */
void Initialize_Display_Process(name)
char *name;
{
  int crv, cbu, cgu, cgv;
  int Y, Cb, Cr, R, G, B;
  int i;
  char dummy;
  int screen;
  Colormap cmap;
  int private;
  XColor xcolor;
  unsigned int fg, bg;
  char *hello = "MPEG-2 Display";
  XSizeHints hint;
  XVisualInfo vinfo;
  XEvent xev;
  unsigned long tmp_pixel;
  XWindowAttributes xwa;

  Display_Ptr = XOpenDisplay(name);

  if (Display_Ptr == NULL)
    Error("Can not open display\n");

  screen = DefaultScreen(Display_Ptr);

  hint.x = 200;
  hint.y = 200;
  hint.width = horizontal_size;
  hint.height = vertical_size;
  hint.flags = PPosition | PSize;

  /* Get some colors */

  bg = WhitePixel (Display_Ptr, screen);
  fg = BlackPixel (Display_Ptr, screen);

  /* Make the window */

  if (!XMatchVisualInfo(Display_Ptr, screen, 8, PseudoColor, &vinfo))
  {
    if (!XMatchVisualInfo(Display_Ptr, screen, 8, GrayScale, &vinfo))
      Error("requires 8 bit display\n");
  }

  Window_Instance = XCreateSimpleWindow (Display_Ptr, DefaultRootWindow (Display_Ptr),
             hint.x, hint.y, hint.width, hint.height, 4, fg, bg);

  XSelectInput(Display_Ptr, Window_Instance, StructureNotifyMask);

  /* Tell other applications about this window */

  XSetStandardProperties (Display_Ptr, Window_Instance, hello, hello, None, NULL, 0, &hint);

  /* Map window. */

  XMapWindow(Display_Ptr, Window_Instance);

  /* Wait for map. */
  do
  {
    XNextEvent(Display_Ptr, &xev);
  }
  while (xev.type != MapNotify || xev.xmap.event != Window_Instance);

  XSelectInput(Display_Ptr, Window_Instance, NoEventMask);

  /* matrix coefficients */
  crv = Inverse_Table_6_9[matrix_coefficients][0];
  cbu = Inverse_Table_6_9[matrix_coefficients][1];
  cgu = Inverse_Table_6_9[matrix_coefficients][2];
  cgv = Inverse_Table_6_9[matrix_coefficients][3];

  /* allocate colors */

  GC_Instance = DefaultGC(Display_Ptr, screen);
  cmap = DefaultColormap(Display_Ptr, screen);
  private = 0;

  /* color allocation:
   * i is the (internal) 8 bit color number, it consists of separate
   * bit fields for Y, U and V: i = (yyyyuuvv), we don't use yyyy=0000
   * and yyyy=1111, this leaves 32 colors for other applications
   *
   * the allocated colors correspond to the following Y, U and V values:
   * Y:   24, 40, 56, 72, 88, 104, 120, 136, 152, 168, 184, 200, 216, 232
   * U,V: -48, -16, 16, 48
   *
   * U and V values span only about half the color space; this gives
   * usually much better quality, although highly saturated colors can
   * not be displayed properly
   *
   * translation to R,G,B is implicitly done by the color look-up table
   */
  for (i=16; i<240; i++)
  {
    /* color space conversion */
    Y  = 16*((i>>4)&15) + 8;
    Cb = 32*((i>>2)&3)  - 48;
    Cr = 32*(i&3)       - 48;

    Y = 76309 * (Y - 16); /* (255/219)*65536 */

    R = Clip[(Y + crv*Cr + 32768)>>16];
    G = Clip[(Y - cgu*Cb - cgv*Cr + 32768)>>16];
    B = Clip[(Y + cbu*Cb + 32786)>>16];

    /* X11 colors are 16 bit */
    xcolor.red   = R << 8;
    xcolor.green = G << 8;
    xcolor.blue  = B << 8;

    if (XAllocColor(Display_Ptr, cmap, &xcolor) != 0)
      Pixel[i] = xcolor.pixel;
    else
    {
      /* allocation failed, have to use a private colormap */

      if (private)
        Error("Couldn't allocate private colormap");

      private = 1;

      if (!Quiet_Flag)
        printf( "Using private colormap (%d colors were available).\n",
          i-16);

      /* Free colors. */
      while (--i >= 16)
      {
        tmp_pixel = Pixel[i]; /* because XFreeColors expects unsigned long */
        XFreeColors(Display_Ptr, cmap, &tmp_pixel, 1, 0);
      }

      /* i is now 15, this restarts the outer loop */

      /* create private colormap */

      XGetWindowAttributes(Display_Ptr, Window_Instance, &xwa);
      cmap = XCreateColormap(Display_Ptr, Window_Instance, xwa.visual, AllocNone);
      XSetWindowColormap(Display_Ptr, Window_Instance, cmap);
    }
  }

#ifdef SH_MEM
  if (XShmQueryExtension(Display_Ptr))
    Shmem_Flag = 1;
  else
  {
    Shmem_Flag = 0;
    if (!Quiet_Flag)
      printf( "Shared memory not supported\nReverting to normal Xlib\n");
  }

  if (Shmem_Flag)
    CompletionType = XShmGetEventBase(Display_Ptr) + ShmCompletion;

  InstallXErrorHandler();

  if (Shmem_Flag)
  {

    Ximage_Ptr = XShmCreateImage(Display_Ptr, None, 8, ZPixmap, NULL,
                             &Shminfo1,
                             Coded_Picture_Width, Coded_Picture_Height);

    if (!progressive_sequence)
      Ximage_Ptr2 = XShmCreateImage(Display_Ptr, None, 8, ZPixmap, NULL,
                                &Shminfo2,
                                Coded_Picture_Width, Coded_Picture_Height);

    /* If no go, then revert to normal Xlib calls. */

    if (Ximage_Ptr==NULL || (!progressive_sequence && Ximage_Ptr2==NULL))
    {
      if (Ximage_Ptr!=NULL)
        XDestroyImage(Ximage_Ptr);
      if (!progressive_sequence && Ximage_Ptr2!=NULL)
        XDestroyImage(Ximage_Ptr2);
      if (!Quiet_Flag)
        printf( "Shared memory error, disabling (Ximage error)\n");
      goto shmemerror;
    }

    /* Success here, continue. */

    Shminfo1.shmid = shmget(IPC_PRIVATE, 
                            Ximage_Ptr->bytes_per_line * Ximage_Ptr->height,
                            IPC_CREAT | 0777);
    if (!progressive_sequence)
      Shminfo2.shmid = shmget(IPC_PRIVATE, 
                              Ximage_Ptr2->bytes_per_line * Ximage_Ptr2->height,
                              IPC_CREAT | 0777);

    if (Shminfo1.shmid<0 || (!progressive_sequence && Shminfo2.shmid<0))
    {
      XDestroyImage(Ximage_Ptr);
      if (!progressive_sequence)
        XDestroyImage(Ximage_Ptr2);
      if (!Quiet_Flag)
        printf( "Shared memory error, disabling (seg id error)\n");
      goto shmemerror;
    }

    Shminfo1.shmaddr = (char *) shmat(Shminfo1.shmid, 0, 0);
    Shminfo2.shmaddr = (char *) shmat(Shminfo2.shmid, 0, 0);

    if (Shminfo1.shmaddr==((char *) -1) ||
        (!progressive_sequence && Shminfo2.shmaddr==((char *) -1)))
    {
      XDestroyImage(Ximage_Ptr);
      if (Shminfo1.shmaddr!=((char *) -1))
        shmdt(Shminfo1.shmaddr);
      if (!progressive_sequence)
      {
        XDestroyImage(Ximage_Ptr2);
        if (Shminfo2.shmaddr!=((char *) -1))
          shmdt(Shminfo2.shmaddr);
      }
      if (!Quiet_Flag)
      {
        printf( "Shared memory error, disabling (address error)\n");
      }
      goto shmemerror;
    }

    Ximage_Ptr->data = Shminfo1.shmaddr;
    Dithered_Image = (unsigned char *)Ximage_Ptr->data;
    Shminfo1.readOnly = False;
    XShmAttach(Display_Ptr, &Shminfo1);
    if (!progressive_sequence)
    {
      Ximage_Ptr2->data = Shminfo2.shmaddr;
      Dithered_Image2 = (unsigned char *)Ximage_Ptr2->data;
      Shminfo2.readOnly = False;
      XShmAttach(Display_Ptr, &Shminfo2);
    }

    XSync(Display_Ptr, False);

    if (gXErrorFlag)
    {
      /* Ultimate failure here. */
      XDestroyImage(Ximage_Ptr);
      shmdt(Shminfo1.shmaddr);
      if (!progressive_sequence)
      {
        XDestroyImage(Ximage_Ptr2);
        shmdt(Shminfo2.shmaddr);
      }
      if (!Quiet_Flag)
        printf( "Shared memory error, disabling.\n");
      gXErrorFlag = 0;
      goto shmemerror;
    }
    else
    {
      shmctl(Shminfo1.shmid, IPC_RMID, 0);
      if (!progressive_sequence)
        shmctl(Shminfo2.shmid, IPC_RMID, 0);
    }

    if (!Quiet_Flag)
    {
      printf( "Sharing memory.\n");
    }
  }
  else
  {
shmemerror:
    Shmem_Flag = 0;
#endif

    Ximage_Ptr = XCreateImage(Display_Ptr,None,8,ZPixmap,0,&dummy,
                          Coded_Picture_Width,Coded_Picture_Height,8,0);

    if (!(Dithered_Image = (unsigned char *)malloc(Coded_Picture_Width*
                                                   Coded_Picture_Height)))
      Error("malloc failed");

    if (!progressive_sequence)
    {
      Ximage_Ptr2 = XCreateImage(Display_Ptr,None,8,ZPixmap,0,&dummy,
                             Coded_Picture_Width,Coded_Picture_Height,8,0);

      if (!(Dithered_Image2 = (unsigned char *)malloc(Coded_Picture_Width*
                                                      Coded_Picture_Height)))
        Error("malloc failed");
    }

#ifdef SH_MEM
  }

  DeInstallXErrorHandler();
#endif
}

void Terminate_Display_Process()
{
#ifdef SH_MEM
  if (Shmem_Flag)
  {
    XShmDetach(Display_Ptr, &Shminfo1);
    XDestroyImage(Ximage_Ptr);
    shmdt(Shminfo1.shmaddr);
    if (!progressive_sequence)
    {
      XShmDetach(Display_Ptr, &Shminfo2);
      XDestroyImage(Ximage_Ptr2);
      shmdt(Shminfo2.shmaddr);
    }
  }
#endif
}

static void Display_Image(Ximage_Ptr,Dithered_Image)
XImage *Ximage_Ptr;
unsigned char *Dithered_Image;
{
  /* display dithered image */
#ifdef SH_MEM
  if (Shmem_Flag)
  {
    XShmPutImage(Display_Ptr, Window_Instance, GC_Instance, Ximage_Ptr, 
       	         0, 0, 0, 0, Ximage_Ptr->width, Ximage_Ptr->height, True);
    XFlush(Display_Ptr);
      
    while (1)
    {
      XEvent xev;
	
      XNextEvent(Display_Ptr, &xev);
      if (xev.type == CompletionType)
        break;
    }
  }
  else 
#endif
  {
    Ximage_Ptr->data = (char *) Dithered_Image; 
    XPutImage(Display_Ptr, Window_Instance, GC_Instance, Ximage_Ptr, 0, 0, 0, 0, Ximage_Ptr->width, Ximage_Ptr->height);
  }
}

void Display_Second_Field()
{
  Display_Image(Ximage_Ptr2,Dithered_Image2);
}

/* 4x4 ordered dither
 *
 * threshold pattern:
 *   0  8  2 10
 *  12  4 14  6
 *   3 11  1  9
 *  15  7 13  5
 */

void Initialize_Dither_Matrix()
{
  int i, v;

  for (i=-8; i<256+8; i++)
  {
    v = i>>4;
    if (v<1)
      v = 1;
    else if (v>14)
      v = 14;
    Y_Table[i+8] = v<<4;
  }

  for (i=0; i<128+16; i++)
  {
    v = (i-40)>>4;
    if (v<0)
      v = 0;
    else if (v>3)
      v = 3;
    Cb_Table[i] = v<<2;
    Cr_Table[i] = v;
  }
}

void dither(src)
unsigned char *src[];
{
  /* should this test only the display flag, not progressive_sequence ? --CF */
  /* CHANGE 95/05/13: progressive_sequence -> progressive_frame */

  if( progressive_frame || Display_Progressive_Flag)
    Dither_Frame(src);
  else
  {
    if ((picture_structure==FRAME_PICTURE && top_field_first) || picture_structure==BOTTOM_FIELD)
    {
      /* top field first */
      if (chroma_format==CHROMA420 && hiQdither)
      {
        Dither_Top_Field420(src,Dithered_Image);
        Dither_Bottom_Field420(src,Dithered_Image2);
      }
      else
      {
        Dither_Top_Field(src,Dithered_Image);
        Dither_Bottom_Field(src,Dithered_Image2);
      }
    }
    else
    {
      /* bottom field first */
      if (chroma_format==CHROMA420 && hiQdither)
      {
        Dither_Bottom_Field420(src,Dithered_Image);
        Dither_Top_Field420(src,Dithered_Image2);
      }
      else
      {
        Dither_Bottom_Field(src,Dithered_Image);
        Dither_Top_Field(src,Dithered_Image2);
      }
    }
  }

  Display_Image(Ximage_Ptr,Dithered_Image);
}

static void Dither_Frame(src)
unsigned char *src[];
{
  int i,j;
  int y,u,v;
  unsigned char *py,*pu,*pv,*dst;

  py = src[0];
  pu = src[1];
  pv = src[2];
  dst = Dithered_Image;

  for (j=0; j<Coded_Picture_Height; j+=4)
  {
    /* line j + 0 */
    for (i=0; i<Coded_Picture_Width; i+=4)
    {
      y = *py++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++ = Pixel[Y_Table[y]|Cb_Table[u]|Cr_Table[v]];
      y = *py++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++ = Pixel[Y_Table[y+8]|Cb_Table[u+8]|Cr_Table[v+8]];
      y = *py++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++ = Pixel[Y_Table[y+2]|Cb_Table[u+2]|Cr_Table[v+2]];
      y = *py++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++ = Pixel[Y_Table[y+10]|Cb_Table[u+10]|Cr_Table[v+10]];
    }

    if (chroma_format==CHROMA420)
    {
      pu -= Chroma_Width;
      pv -= Chroma_Width;
    }

    /* line j + 1 */
    for (i=0; i<Coded_Picture_Width; i+=4)
    {
      y = *py++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++ = Pixel[Y_Table[y+12]|Cb_Table[u+12]|Cr_Table[v+12]];
      y = *py++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++ = Pixel[Y_Table[y+4]|Cb_Table[u+4]|Cr_Table[v+4]];
      y = *py++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++ = Pixel[Y_Table[y+14]|Cb_Table[u+14]|Cr_Table[v+14]];
      y = *py++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++ = Pixel[Y_Table[y+6]|Cb_Table[u+6]|Cr_Table[v+6]];
    }

    /* line j + 2 */
    for (i=0; i<Coded_Picture_Width; i+=4)
    {
      y = *py++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++ = Pixel[Y_Table[y+3]|Cb_Table[u+3]|Cr_Table[v+3]];
      y = *py++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++ = Pixel[Y_Table[y+11]|Cb_Table[u+11]|Cr_Table[v+11]];
      y = *py++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++ = Pixel[Y_Table[y+1]|Cb_Table[u+1]|Cr_Table[v+1]];
      y = *py++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++ = Pixel[Y_Table[y+9]|Cb_Table[u+9]|Cr_Table[v+9]];
    }

    if (chroma_format==CHROMA420)
    {
      pu -= Chroma_Width;
      pv -= Chroma_Width;
    }

    /* line j + 3 */
    for (i=0; i<Coded_Picture_Width; i+=4)
    {
      y = *py++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++ = Pixel[Y_Table[y+15]|Cb_Table[u+15]|Cr_Table[v+15]];
      y = *py++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++ = Pixel[Y_Table[y+7]|Cb_Table[u+7]|Cr_Table[v+7]];
      y = *py++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++ = Pixel[Y_Table[y+13]|Cb_Table[u+13]|Cr_Table[v+13]];
      y = *py++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++ = Pixel[Y_Table[y+5]|Cb_Table[u+5]|Cr_Table[v+5]];
    }
  }

}

static void Dither_Top_Field(src,dst)
unsigned char *src[];
unsigned char *dst;
{
  int i,j;
  int y,Y2,u,v;
  unsigned char *py,*Y2_ptr,*pu,*pv,*dst2;

  py = src[0];
  Y2_ptr = src[0] + (Coded_Picture_Width<<1);
  pu = src[1];
  pv = src[2];
  dst2 = dst + Coded_Picture_Width;

  for (j=0; j<Coded_Picture_Height; j+=4)
  {
    /* line j + 0, j + 1 */
    for (i=0; i<Coded_Picture_Width; i+=4)
    {
      y = *py++;
      Y2 = *Y2_ptr++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++  = Pixel[Y_Table[y]|Cb_Table[u]|Cr_Table[v]];
      *dst2++ = Pixel[Y_Table[((y+Y2)>>1)+12]|Cb_Table[u+12]|Cr_Table[v+12]];

      y = *py++;
      Y2 = *Y2_ptr++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++  = Pixel[Y_Table[y+8]|Cb_Table[u+8]|Cr_Table[v+8]];
      *dst2++ = Pixel[Y_Table[((y+Y2)>>1)+4]|Cb_Table[u+4]|Cr_Table[v+4]];

      y = *py++;
      Y2 = *Y2_ptr++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++  = Pixel[Y_Table[y+2]|Cb_Table[u+2]|Cr_Table[v+2]];
      *dst2++ = Pixel[Y_Table[((y+Y2)>>1)+14]|Cb_Table[u+14]|Cr_Table[v+14]];

      y = *py++;
      Y2 = *Y2_ptr++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++  = Pixel[Y_Table[y+10]|Cb_Table[u+10]|Cr_Table[v+10]];
      *dst2++ = Pixel[Y_Table[((y+Y2)>>1)+6]|Cb_Table[u+6]|Cr_Table[v+6]];
    }

    py += Coded_Picture_Width;

    if (j!=(Coded_Picture_Height-4))
      Y2_ptr += Coded_Picture_Width;
    else
      Y2_ptr -= Coded_Picture_Width;

    dst += Coded_Picture_Width;
    dst2 += Coded_Picture_Width;

    if (chroma_format==CHROMA420)
    {
      pu -= Chroma_Width;
      pv -= Chroma_Width;
    }
    else
    {
      pu += Chroma_Width;
      pv += Chroma_Width;
    }

    /* line j + 2, j + 3 */
    for (i=0; i<Coded_Picture_Width; i+=4)
    {
      y = *py++;
      Y2 = *Y2_ptr++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++  = Pixel[Y_Table[y+3]|Cb_Table[u+3]|Cr_Table[v+3]];
      *dst2++ = Pixel[Y_Table[((y+Y2)>>1)+15]|Cb_Table[u+15]|Cr_Table[v+15]];

      y = *py++;
      Y2 = *Y2_ptr++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++  = Pixel[Y_Table[y+11]|Cb_Table[u+11]|Cr_Table[v+11]];
      *dst2++ = Pixel[Y_Table[((y+Y2)>>1)+7]|Cb_Table[u+7]|Cr_Table[v+7]];

      y = *py++;
      Y2 = *Y2_ptr++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++  = Pixel[Y_Table[y+1]|Cb_Table[u+1]|Cr_Table[v+1]];
      *dst2++ = Pixel[Y_Table[((y+Y2)>>1)+13]|Cb_Table[u+13]|Cr_Table[v+13]];

      y = *py++;
      Y2 = *Y2_ptr++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++  = Pixel[Y_Table[y+9]|Cb_Table[u+9]|Cr_Table[v+9]];
      *dst2++ = Pixel[Y_Table[((y+Y2)>>1)+5]|Cb_Table[u+5]|Cr_Table[v+5]];
    }

    py += Coded_Picture_Width;
    Y2_ptr += Coded_Picture_Width;
    dst += Coded_Picture_Width;
    dst2 += Coded_Picture_Width;
    pu += Chroma_Width;
    pv += Chroma_Width;
  }
}

static void Dither_Bottom_Field(src,dst)
unsigned char *src[];
unsigned char *dst;
{
  int i,j;
  int y,Y2,u,v;
  unsigned char *py,*Y2_ptr,*pu,*pv,*dst2;

  py = src[0] + Coded_Picture_Width;
  Y2_ptr = py;
  pu = src[1] + Chroma_Width;
  pv = src[2] + Chroma_Width;
  dst2 = dst + Coded_Picture_Width;

  for (j=0; j<Coded_Picture_Height; j+=4)
  {
    /* line j + 0, j + 1 */
    for (i=0; i<Coded_Picture_Width; i+=4)
    {
      y = *py++;
      Y2 = *Y2_ptr++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++  = Pixel[Y_Table[((y+Y2)>>1)]|Cb_Table[u]|Cr_Table[v]];
      *dst2++ = Pixel[Y_Table[Y2+12]|Cb_Table[u+12]|Cr_Table[v+12]];

      y = *py++;
      Y2 = *Y2_ptr++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++  = Pixel[Y_Table[((y+Y2)>>1)+8]|Cb_Table[u+8]|Cr_Table[v+8]];
      *dst2++ = Pixel[Y_Table[Y2+4]|Cb_Table[u+4]|Cr_Table[v+4]];

      y = *py++;
      Y2 = *Y2_ptr++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++  = Pixel[Y_Table[((y+Y2)>>1)+2]|Cb_Table[u+2]|Cr_Table[v+2]];
      *dst2++ = Pixel[Y_Table[Y2+14]|Cb_Table[u+14]|Cr_Table[v+14]];

      y = *py++;
      Y2 = *Y2_ptr++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++  = Pixel[Y_Table[((y+Y2)>>1)+10]|Cb_Table[u+10]|Cr_Table[v+10]];
      *dst2++ = Pixel[Y_Table[Y2+6]|Cb_Table[u+6]|Cr_Table[v+6]];
    }

    if (j==0)
      py -= Coded_Picture_Width;
    else
      py += Coded_Picture_Width;

    Y2_ptr += Coded_Picture_Width;
    dst += Coded_Picture_Width;
    dst2 += Coded_Picture_Width;

    if (chroma_format==CHROMA420)
    {
      pu -= Chroma_Width;
      pv -= Chroma_Width;
    }
    else
    {
      pu += Chroma_Width;
      pv += Chroma_Width;
    }

    /* line j + 2. j + 3 */
    for (i=0; i<Coded_Picture_Width; i+=4)
    {
      y = *py++;
      Y2 = *Y2_ptr++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++  = Pixel[Y_Table[((y+Y2)>>1)+3]|Cb_Table[u+3]|Cr_Table[v+3]];
      *dst2++ = Pixel[Y_Table[Y2+15]|Cb_Table[u+15]|Cr_Table[v+15]];

      y = *py++;
      Y2 = *Y2_ptr++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++  = Pixel[Y_Table[((y+Y2)>>1)+11]|Cb_Table[u+11]|Cr_Table[v+11]];
      *dst2++ = Pixel[Y_Table[Y2+7]|Cb_Table[u+7]|Cr_Table[v+7]];

      y = *py++;
      Y2 = *Y2_ptr++;
      u = *pu++ >> 1;
      v = *pv++ >> 1;
      *dst++  = Pixel[Y_Table[((y+Y2)>>1)+1]|Cb_Table[u+1]|Cr_Table[v+1]];
      *dst2++ = Pixel[Y_Table[Y2+13]|Cb_Table[u+13]|Cr_Table[v+13]];

      y = *py++;
      Y2 = *Y2_ptr++;
      if (chroma_format==CHROMA444)
      {
        u = *pu++ >> 1;
        v = *pv++ >> 1;
      }
      *dst++  = Pixel[Y_Table[((y+Y2)>>1)+9]|Cb_Table[u+9]|Cr_Table[v+9]];
      *dst2++ = Pixel[Y_Table[Y2+5]|Cb_Table[u+5]|Cr_Table[v+5]];
    }

    py += Coded_Picture_Width;
    Y2_ptr += Coded_Picture_Width;
    dst += Coded_Picture_Width;
    dst2 += Coded_Picture_Width;
    pu += Chroma_Width;
    pv += Chroma_Width;
  }
}

static void Dither_Top_Field420(src,dst)
unsigned char *src[];
unsigned char *dst;
{
  int i,j;
  int Y1,Cb1,Cr1,Y2,Cb2,Cr2;
  unsigned char *Y1_ptr,*Cb1_ptr,*Cr1_ptr,*Y2_ptr,*Cb2_ptr,*Cr2_ptr,*dst2;

  Y1_ptr = src[0];
  Cb1_ptr = src[1];
  Cr1_ptr = src[2];

  Y2_ptr = Y1_ptr + (Coded_Picture_Width<<1);
  Cb2_ptr = Cb1_ptr + (Chroma_Width<<1);
  Cr2_ptr = Cr1_ptr + (Chroma_Width<<1);

  dst2 = dst + Coded_Picture_Width;

  for (j=0; j<Coded_Picture_Height; j+=4)
  {
    /* line j + 0, j + 1 */
    for (i=0; i<Coded_Picture_Width; i+=4)
    {
      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      Cb1 = *Cb1_ptr++ >> 1;
      Cr1 = *Cr1_ptr++ >> 1;
      Cb2 = *Cb2_ptr++ >> 1;
      Cr2 = *Cr2_ptr++ >> 1;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)]|Cb_Table[Cb1]|Cr_Table[Cr1]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)+12]|Cb_Table[((3*Cb1+Cb2)>>2)+12]
                                             |Cr_Table[((3*Cr1+Cr2)>>2)+12]];

      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+8]|Cb_Table[Cb1+8]|Cr_Table[Cr1+8]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)+4]|Cb_Table[((3*Cb1+Cb2)>>2)+4]
                                            |Cr_Table[((3*Cr1+Cr2)>>2)+4]];

      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      Cb1 = *Cb1_ptr++ >> 1;
      Cr1 = *Cr1_ptr++ >> 1;
      Cb2 = *Cb2_ptr++ >> 1;
      Cr2 = *Cr2_ptr++ >> 1;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+2]|Cb_Table[Cb1+2]|Cr_Table[Cr1+2]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)+14]|Cb_Table[((3*Cb1+Cb2)>>2)+14]
                                             |Cr_Table[((3*Cr1+Cr2)>>2)+14]];

      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+10]|Cb_Table[Cb1+10]|Cr_Table[Cr1+10]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)+6]|Cb_Table[((3*Cb1+Cb2)>>2)+6]
                                            |Cr_Table[((3*Cr1+Cr2)>>2)+6]];
    }

    Y1_ptr += Coded_Picture_Width;

    if (j!=(Coded_Picture_Height-4))
      Y2_ptr += Coded_Picture_Width;
    else
      Y2_ptr -= Coded_Picture_Width;

    Cb1_ptr -= Chroma_Width;
    Cr1_ptr -= Chroma_Width;
    Cb2_ptr -= Chroma_Width;
    Cr2_ptr -= Chroma_Width;

    dst  += Coded_Picture_Width;
    dst2 += Coded_Picture_Width;

    /* line j + 2, j + 3 */
    for (i=0; i<Coded_Picture_Width; i+=4)
    {
      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      Cb1 = *Cb1_ptr++ >> 1;
      Cr1 = *Cr1_ptr++ >> 1;
      Cb2 = *Cb2_ptr++ >> 1;
      Cr2 = *Cr2_ptr++ >> 1;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+3]|Cb_Table[((Cb1+Cb2)>>1)+3]
                                            |Cr_Table[((Cr1+Cr2)>>1)+3]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)+15]|Cb_Table[((Cb1+3*Cb2)>>2)+15]
                                             |Cr_Table[((Cr1+3*Cr2)>>2)+15]];

      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+11]|Cb_Table[((Cb1+Cb2)>>1)+11]
                                             |Cr_Table[((Cr1+Cr2)>>1)+11]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)+7]|Cb_Table[((Cb1+3*Cb2)>>2)+7]
                                            |Cr_Table[((Cr1+3*Cr2)>>2)+7]];

      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      Cb1 = *Cb1_ptr++ >> 1;
      Cr1 = *Cr1_ptr++ >> 1;
      Cb2 = *Cb2_ptr++ >> 1;
      Cr2 = *Cr2_ptr++ >> 1;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+1]|Cb_Table[((Cb1+Cb2)>>1)+1]
                                            |Cr_Table[((Cr1+Cr2)>>1)+1]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)+13]|Cb_Table[((Cb1+3*Cb2)>>2)+13]
                                             |Cr_Table[((Cr1+3*Cr2)>>2)+13]];

      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+9]|Cb_Table[((Cb1+Cb2)>>1)+9]
                                            |Cr_Table[((Cr1+Cr2)>>1)+9]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)+5]|Cb_Table[((Cb1+3*Cb2)>>2)+5]
                                            |Cr_Table[((Cr1+3*Cr2)>>2)+5]];
    }

    Y1_ptr += Coded_Picture_Width;
    Y2_ptr += Coded_Picture_Width;
    Cb1_ptr += Chroma_Width;
    Cr1_ptr += Chroma_Width;
    if (j!=(Coded_Picture_Height-8))
    {
      Cb2_ptr += Chroma_Width;
      Cr2_ptr += Chroma_Width;
    }
    else
    {
      Cb2_ptr -= Chroma_Width;
      Cr2_ptr -= Chroma_Width;
    }
    dst += Coded_Picture_Width;
    dst2+= Coded_Picture_Width;
  }
}

static void Dither_Bottom_Field420(src,dst)
unsigned char *src[];
unsigned char *dst;
{
  int i,j;
  int Y1,Cb1,Cr1,Y2,Cb2,Cr2;
  unsigned char *Y1_ptr,*Cb1_ptr,*Cr1_ptr,*Y2_ptr,*Cb2_ptr,*Cr2_ptr,*dst2;

  Y2_ptr = Y1_ptr = src[0] + Coded_Picture_Width;
  Cb2_ptr = Cb1_ptr = src[1] + Chroma_Width;
  Cr2_ptr = Cr1_ptr = src[2] + Chroma_Width;

  dst2 = dst;

  for (j=0; j<Coded_Picture_Height; j+=4)
  {
    /* line j + 0, j + 1 */
    for (i=0; i<Coded_Picture_Width; i+=4)
    {
      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      Cb1 = *Cb1_ptr++ >> 1;
      Cr1 = *Cr1_ptr++ >> 1;
      Cb2 = *Cb2_ptr++ >> 1;
      Cr2 = *Cr2_ptr++ >> 1;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+15]|Cb_Table[((3*Cb1+Cb2)>>2)+15]
                                             |Cr_Table[((3*Cr1+Cr2)>>2)+15]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)]|Cb_Table[((Cb1+Cb2)>>1)]
                                          |Cr_Table[((Cr1+Cr2)>>1)]];

      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+7]|Cb_Table[((3*Cb1+Cb2)>>2)+7]
                                            |Cr_Table[((3*Cr1+Cr2)>>2)+7]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)+8]|Cb_Table[((Cb1+Cb2)>>1)+8]
                                            |Cr_Table[((Cr1+Cr2)>>1)+8]];

      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      Cb1 = *Cb1_ptr++ >> 1;
      Cr1 = *Cr1_ptr++ >> 1;
      Cb2 = *Cb2_ptr++ >> 1;
      Cr2 = *Cr2_ptr++ >> 1;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+13]|Cb_Table[((3*Cb1+Cb2)>>2)+13]
                                             |Cr_Table[((3*Cr1+Cr2)>>2)+13]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)+2]|Cb_Table[((Cb1+Cb2)>>1)+2]
                                            |Cr_Table[((Cr1+Cr2)>>1)+2]];

      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+5]|Cb_Table[((3*Cb1+Cb2)>>2)+5]
                                            |Cr_Table[((3*Cr1+Cr2)>>2)+5]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)+10]|Cb_Table[((Cb1+Cb2)>>1)+10]
                                             |Cr_Table[((Cr1+Cr2)>>1)+10]];
    }

    if (j!=0)
      Y1_ptr += Coded_Picture_Width;
    else
      Y1_ptr -= Coded_Picture_Width;

    Y2_ptr += Coded_Picture_Width;

    Cb1_ptr -= Chroma_Width;
    Cr1_ptr -= Chroma_Width;
    Cb2_ptr -= Chroma_Width;
    Cr2_ptr -= Chroma_Width;

    if (j!=0)
      dst  += Coded_Picture_Width;

    dst2 += Coded_Picture_Width;

    /* line j + 2, j + 3 */
    for (i=0; i<Coded_Picture_Width; i+=4)
    {
      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      Cb1 = *Cb1_ptr++ >> 1;
      Cr1 = *Cr1_ptr++ >> 1;
      Cb2 = *Cb2_ptr++ >> 1;
      Cr2 = *Cr2_ptr++ >> 1;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+12]|Cb_Table[((Cb1+3*Cb2)>>2)+12]
                                             |Cr_Table[((Cr1+3*Cr2)>>2)+12]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)+3]|Cb_Table[Cb2+3]
                                            |Cr_Table[Cr2+3]];

      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+4]|Cb_Table[((Cb1+3*Cb2)>>2)+4]
                                            |Cr_Table[((Cr1+3*Cr2)>>2)+4]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)+11]|Cb_Table[Cb2+11]
                                             |Cr_Table[Cr2+11]];

      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      Cb1 = *Cb1_ptr++ >> 1;
      Cr1 = *Cr1_ptr++ >> 1;
      Cb2 = *Cb2_ptr++ >> 1;
      Cr2 = *Cr2_ptr++ >> 1;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+14]|Cb_Table[((Cb1+3*Cb2)>>2)+14]
                                             |Cr_Table[((Cr1+3*Cr2)>>2)+14]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)+1]|Cb_Table[Cb2+1]
                                            |Cr_Table[Cr2+1]];

      Y1 = *Y1_ptr++;
      Y2 = *Y2_ptr++;
      *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+6]|Cb_Table[((Cb1+3*Cb2)>>2)+6]
                                            |Cr_Table[((Cr1+3*Cr2)>>2)+6]];
      *dst2++ = Pixel[Y_Table[((Y1+3*Y2)>>2)+9]|Cb_Table[Cb2+9]
                                            |Cr_Table[Cr2+9]];
    }

    Y1_ptr += Coded_Picture_Width;
    Y2_ptr += Coded_Picture_Width;

    if (j!=0)
    {
      Cb1_ptr += Chroma_Width;
      Cr1_ptr += Chroma_Width;
    }
    else
    {
      Cb1_ptr -= Chroma_Width;
      Cr1_ptr -= Chroma_Width;
    }

    Cb2_ptr += Chroma_Width;
    Cr2_ptr += Chroma_Width;

    dst += Coded_Picture_Width;
    dst2+= Coded_Picture_Width;
  }

  Y2_ptr -= (Coded_Picture_Width<<1);
  Cb2_ptr -= (Chroma_Width<<1);
  Cr2_ptr -= (Chroma_Width<<1);

  /* dither last line */
  for (i=0; i<Coded_Picture_Width; i+=4)
  {
    Y1 = *Y1_ptr++;
    Y2 = *Y2_ptr++;
    Cb1 = *Cb1_ptr++ >> 1;
    Cr1 = *Cr1_ptr++ >> 1;
    Cb2 = *Cb2_ptr++ >> 1;
    Cr2 = *Cr2_ptr++ >> 1;
    *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+15]|Cb_Table[((3*Cb1+Cb2)>>2)+15]
                                           |Cr_Table[((3*Cr1+Cr2)>>2)+15]];

    Y1 = *Y1_ptr++;
    Y2 = *Y2_ptr++;
    *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+7]|Cb_Table[((3*Cb1+Cb2)>>2)+7]
                                          |Cr_Table[((3*Cr1+Cr2)>>2)+7]];

    Y1 = *Y1_ptr++;
    Y2 = *Y2_ptr++;
    Cb1 = *Cb1_ptr++ >> 1;
    Cr1 = *Cr1_ptr++ >> 1;
    Cb2 = *Cb2_ptr++ >> 1;
    Cr2 = *Cr2_ptr++ >> 1;
    *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+13]|Cb_Table[((3*Cb1+Cb2)>>2)+13]
                                           |Cr_Table[((3*Cr1+Cr2)>>2)+13]];

    Y1 = *Y1_ptr++;
    Y2 = *Y2_ptr++;
    *dst++  = Pixel[Y_Table[((3*Y1+Y2)>>2)+5]|Cb_Table[((3*Cb1+Cb2)>>2)+5]
                                          |Cr_Table[((3*Cr1+Cr2)>>2)+5]];
    }

}
#endif
