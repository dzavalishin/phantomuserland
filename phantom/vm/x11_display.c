/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * This source file implements (attempts to) X11 based wrapper for VM to
 * run in Unix-hosted environment.
 *
 * TODO: GBR -> RGB!!
 *
 **/


#include <X11/Xlib.h> // preceede most other headers!
#include <X11/Xutil.h>		/* BitmapOpenFailed, etc. */

#include "x11_screen.h"



static Display *display = 0;
static Window win;
static GC gc;
static int xsize;
static int ysize;
static int screenNumber = 0;
static unsigned long white;
static unsigned long black;

static char *x11_videomem;


static XImage *image;
static void * x11_prepare(Display *display, Window win, GC gc)
{
    x11_videomem = calloc( 4, xsize * ysize );
    if( x11_videomem == 0 ) return 0;

    image = XCreateImage (display,
                                  CopyFromParent, 24,
                                  ZPixmap, 0,
                                  (char *) x11_videomem,
                                  xsize, ysize,
                                  32, 0 );

    //memset( x11_videomem, 4 * xsize * ysize, 0xFF );
    int size = xsize * ysize;
    int *p = (int *)x11_videomem;
    while(size--)
    {
        //*p++ = 0x00FF0000;
        *p++ = 0x00655555;
    }

    XPutImage(display, win, gc, image, 0, 0, 0, 0, xsize, ysize);
    return x11_videomem;
}



int win_x11_init(int _xsize, int _ysize)
{
    xsize = _xsize;
    ysize = _ysize;

    display = XOpenDisplay( NULL );
    if( !display ){ return 1; }

    screenNumber = DefaultScreen(display);
    white = WhitePixel(display,screenNumber);
    black = BlackPixel(display,screenNumber);

    win = XCreateSimpleWindow(display,
                                     DefaultRootWindow(display),
                                     50, 50,   // origin
                                     xsize, ysize, // size
                                     0, black, // border
                                     white );  // backgd

    XMapWindow( display, win );

    long eventMask = StructureNotifyMask;
    XSelectInput( display, win, eventMask );

    XEvent evt;
    do{
        XNextEvent( display, &evt );   // calls XFlush
    }while( evt.type != MapNotify );


    gc = XCreateGC( display, win,
                       0,        // mask of values
                       NULL );   // array of values


    void * vmem = x11_prepare( display, win, gc);

    //drv_video_x11.screen = vmem;

    win_x11_set_screen( vmem );

    return 0;
}

//void * win_x11_update(const char *src)
void win_x11_update(void)
{
/*
    int pixels = xsize * ysize;

    int *dst;
    while(pixels--)
        *dst = *src;
*/
    XPutImage(display, win, gc, image, 0, 0, 0, 0, xsize, ysize);
}




void win_x11_message_loop(void)
{
    XEvent evt;

    long eventMask =
        ButtonPressMask | ButtonReleaseMask | OwnerGrabButtonMask | PointerMotionMask |
        KeyPressMask | KeyReleaseMask
        ;

    XSelectInput(display,win,eventMask); // override prev

    do {
        XNextEvent( display, &evt );   // calls XFlush()
#if 1
        switch( evt.type )
        {
        case Expose: // TODO we can repaint win part easily
            win_x11_update();
            break;

        case MotionNotify:
            {
                XMotionEvent *e = (XMotionEvent *) &evt;

                win_x11_mouse_event( e->x, e->y, e->state );
            }

        case KeyPress:
        case KeyRelease:
            {
                int isRelease = evt.type == KeyRelease;

                XKeyEvent *e = (XKeyEvent *) &evt;

                win_x11_key_event( e->x, e->y, e->state, e->keycode, isRelease );
            }
            break;

        case ButtonPress:
        case ButtonRelease:
            {
                XButtonEvent *e = (XButtonEvent *) &evt;

                win_x11_mouse_event( e->x, e->y, e->state );
            }
            break;

        case EnterNotify:
            printf("Enter\n");
            break;
        }
#endif
    }
    //while( evt.type != ButtonRelease );
    while( 1 );

}


