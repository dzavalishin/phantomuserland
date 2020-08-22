/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: no
 * Preliminary: no
 *
 * This source file implements (attempts to) X11 based wrapper for VM to
 * run in Unix-hosted environment.
 *
 * TODO: GBR -> RGB!!
 *
 **/

#include <X11/Xlib.h> // preceede most other headers!
#include <X11/Xutil.h>		/* BitmapOpenFailed, etc. */

#include "screen_x11.h"


void drawbmp(Display *display, Window win, GC gc)
{

        /* this variable will contain the ID of the newly created pixmap.    */
        Pixmap bitmap;
        /* these variables will contain the dimensions of the loaded bitmap. */
        unsigned int bitmap_width, bitmap_height;
        /* these variables will contain the location of the hotspot of the   */
        /* loaded bitmap.                                                    */
        int hotspot_x, hotspot_y;

#if 0
        {
        /* load the bitmap found in the file "icon.bmp", create a pixmap     */
        /* containing its data in the server, and put its ID in the 'bitmap' */
        /* variable.                                                         */
        int rc = XReadBitmapFile(display, win,
                                 "close.bmp",
                                 &bitmap_width, &bitmap_height,
                                 &bitmap,
                                 &hotspot_x, &hotspot_y);
        /* check for failure or success. */
        switch (rc) {
        case BitmapOpenFailed:
            printf( "XReadBitmapFile - could not open file 'icon.bmp'.\n");
            exit(1);
            break;
        case BitmapFileInvalid:
            printf(
                    "XReadBitmapFile - file '%s' doesn't contain a valid bitmap.\n",
                    "icon.bmp");
            exit(1);
            break;
        case BitmapNoMemory:
            printf( "XReadBitmapFile - not enough memory.\n");
            exit(1);
            break;
        }

        /* start drawing the given pixmap on to our window. */
        {
            int i, j;

            for(i=0; i<6; i++) {
                for(j=0; j<6; j++) {
                    XCopyPlane(display, bitmap, win, gc,
                               0, 0,
                               bitmap_width, bitmap_height,
                               j*bitmap_width, i*bitmap_height,
                               1);
                    XSync(display, False);
                    usleep(100000);
                }
            }
        }
    }
#else

        bitmap = XCreatePixmap(display, XDefaultRootWindow(display), 30, 40, XDefaultDepth(display, XDefaultScreen(display)));

        int screenNumber = DefaultScreen(display);
        unsigned long white = WhitePixel(display,screenNumber);
        unsigned long black = BlackPixel(display,screenNumber);

        XSetForeground( display, gc, black );
        XFillRectangle(display, bitmap, gc, 0, 0, 15, 20);
        XSetForeground( display, gc, white );
        XDrawPoint(display, bitmap, gc, 10, 10);

        XCopyPlane(display, bitmap, win, gc,
             0, 0,
             15, 20,
             15, 20,
             1);




#endif


}

static XImage *image;
static void * prepare(Display *display, Window win, GC gc, int xsize, int ysize)
{
    char *newBuf = calloc( 4, xsize * ysize );
    if( newBuf == 0 ) return 0;

    image = XCreateImage (display,
                                  CopyFromParent, 24,
                                  ZPixmap, 0,
                                  (char *) newBuf,
                                  xsize, ysize,
                                  32, 0 );

    //memset( newBuf, 4 * xsize * ysize, 0xFF );
    int size = xsize * ysize;
    int *p = (int *)newBuf;
    while(size--)
    {
        *p++ = 0x00FF0000;
    }

    XPutImage(display, win, gc, image, 0, 0, 0, 0, xsize, ysize);
    return newBuf;
}



static Display *display = 0;
static Window win;
static GC gc;
static int xsize;
static int ysize;
static int screenNumber = 0;
static unsigned long white;
static unsigned long black;

void * win_x11_update(const char *src)
{
/** dst == ??
    int pixels = xsize * ysize;
 
    int *dst;
    while(pixels--)
        *dst = *src;

        XPutImage(display, win, gc, image, 0, 0, 0, 0, xsize, ysize);
*/
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

    return 0;
}


void win_x11_stop()
{
    if( display == 0 ) return;

    XDestroyWindow( display, win );
    XCloseDisplay( display );
}

int main()
{

    win_x11_init(200,200);




    XSetForeground( display, gc, black );


    XDrawLine(display, win, gc, 10, 10,190,190); //from-to
    XDrawLine(display, win, gc, 10,190,190, 10); //from-to

    //drawbmp( display, win, gc );
    prepare( display, win, gc, 150, 10);

    XEvent evt;
    long eventMask = ButtonPressMask|ButtonReleaseMask;
    XSelectInput(display,win,eventMask); // override prev

    do{
        XNextEvent( display, &evt );   // calls XFlush()
    } while( evt.type != ButtonRelease );


    win_x11_stop();


    return 0;
}



void win_x11_message_loop(void)
{
    XEvent evt;
    long eventMask = ButtonPressMask|ButtonReleaseMask;
    XSelectInput(display,win,eventMask); // override prev

    do{
        XNextEvent( display, &evt );   // calls XFlush()
    } while( evt.type != ButtonRelease );

}


