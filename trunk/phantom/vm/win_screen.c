/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: no, this is not a kernel code.
 *
 * This source file implements Windows based wrapper for VM to
 * run in Windows-hosted environment.
 *
 * TODO: GBR -> RGB!!
 *
**/

#ifndef __linux__

#include <phantom_libc.h>

//#include <windows.h>
#include <string.h>
#include <assert.h>

//#include <drv_video_screen.h>
#include <video/screen.h>
#include <video/internal.h>

#include "gcc_replacements.h"

#include "winhal.h"



struct drv_video_screen_t        drv_video_win32 =
{
    "Windows",
    // size
    0, 0, 24,
    // mouse x y flags
    0, 0, 0,

    // screen
    0,

.probe  = (void *)vid_null,
.start  = (void *)vid_null,
.accel  = (void *)vid_null,
.stop   = (void *)vid_null,

update:(void*)vid_null,
bitblt:(void*)vid_null,
//(void*)vid_null, // was winblt

.readblt = (void*)vid_null,

.mouse               =  (void*)vid_null,

.mouse_redraw_cursor =  vid_mouse_draw_deflt,
.mouse_set_cursor    =  vid_mouse_set_cursor_deflt,
.mouse_disable       =  vid_mouse_off_deflt,
.mouse_enable        =  vid_mouse_on_deflt,

};



/*
const char *CLASSNAME = "Phantom", *WINNAME = "Phantom test environment";

LRESULT CALLBACK WndProc(HWND hWnd, unsigned int iMessage, WPARAM wParam, LPARAM lParam);

static HWND hWnd;

TRACKMOUSEEVENT eventTrack;


static char * win_src_screen_image;
static HBITMAP screenBitmap;

HDC hBitmapDC;
*/


static int init_ok = 0;
static int init_err = 0;


#if 0

//  ---------------- cursor -----------------


int cysize = 60;
int cxsize = 80;
char cwin[ysize*xsize*3];


void pvm_win_init_cursor()
{
    int i;

    for( i = 0; i < 10; i++ )
    {
        // fill
        int c;
        for( c = 0; c < xsize*ysize*3; c++ )
            cwin[c] = (i+2)*10;

        drv_video_bitblt(win, 0, i*10, cxsize, cysize);
        //win_scr_screen_update();
        drv_video_update();
    }
}


void pvm_win_draw_cursor(int x, int y)
{
}




//  -----------------------------------------

#endif


#if 0

//static

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
    HDC hDC;
    PAINTSTRUCT lpPaint;


    switch(iMessage)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        exit(0);
        if(MessageBox(hWnd, "Do you really want to quit?", "Message", MB_YESNO) == IDYES)
            PostQuitMessage(0);
        break;

    case WM_ERASEBKGND:
        return 1; // Done

    case WM_PAINT:
        GdiFlush();
        hDC = BeginPaint( hWnd, &lpPaint);

        // Assume hPaintDC is a variable of type HDC, and the dc we're rendering to
        HDC hBitmapDC = CreateCompatibleDC(hDC);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hBitmapDC, screenBitmap);
        //HPALETTE hOldPalette = SelectPalette(hPaintDC, hPalette, FALSE);
        GdiFlush();
        if( !BitBlt(hDC, 0, 0, VSCREEN_WIDTH, VSCREEN_HEIGHT, hBitmapDC, 0, 0, SRCCOPY) )
        {
            DWORD err = GetLastError();
            //FormatMessage();
            printf("Win error %d", (int)err);

            LPVOID lpMsgBuf;
            FormatMessage(
                          FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,

                          NULL,
                          err,
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                          (LPTSTR) &lpMsgBuf,
                          0, NULL );

            printf("WinErr: %s\n", (const char *)lpMsgBuf );

        }
        GdiFlush();
        //SelectPalette(hPaintDC, hOldPalette, TRUE);
        SelectObject(hBitmapDC, hOldBitmap);
        DeleteDC(hBitmapDC);


        /*if(eline)
         {
         MoveToEx(hDC, 60, 20, NULL);
         LineTo(hDC, 264, 122);
         }*/

        // TODO: paint OS mouse cursor

        EndPaint( hWnd, &lpPaint);

        break;

#if HOVER
    case WM_MOUSEHOVER:
        {
            int xPos = (short)(0x0FFFF & lParam);//GET_X_LPARAM(lParam);
            int yPos = VSCREEN_HEIGHT - (short)(0x0FFFF & (lParam>>16));//GET_Y_LPARAM(lParam);

            printf("%d,%d\n", xPos, yPos );
            TrackMouseEvent(&eventTrack);
        }
        break;
#endif


    //case WM_KEYDOWN:
    //case WM_KEYUP:        TranslateMessage(  __in  const MSG *lpMsg );

    case WM_CHAR:
        {
            printf("-%x-", (int)lParam );
        }
        break;

    case WM_MOUSEMOVE:
        {
            int xPos = (short)(0x0FFFF & lParam);//GET_X_LPARAM(lParam);
            int yPos = VSCREEN_HEIGHT - (short)(0x0FFFF & (lParam>>16));//GET_Y_LPARAM(lParam);

            //	printf("%d,%d\n", xPos, yPos );

            drv_video_win32.mouse_x = xPos;
            drv_video_win32.mouse_y = yPos;
            drv_video_win32.mouse_flags = wParam;
            drv_video_win32.mouse();
#if 1
            struct ui_event e;
            e.type = UI_EVENT_TYPE_MOUSE;
            e.time = fast_time();
            e.focus= 0;

            e.m.buttons = wParam;
            e.abs_x = xPos;
            e.abs_y = VSCREEN_HEIGHT - yPos - 1;

            ev_q_put_any( &e );
            //printf("-ms-");            printf("%d,%d\n", xPos, yPos );
#endif
        }
        break;

    default:
        return DefWindowProc(hWnd, iMessage, wParam, lParam);
    }
    return 0;
}

static int pvm_win_setup_window()
{

    HINSTANCE hInstance = 0;
    WNDCLASS WndClass;
    // NOTE: The memset() here is added to deal with this code crashing on
    // WinNT (reported by Girish Deodhar, solution due to him as well)
    // The extra include statement is for memset() too...
    memset(&WndClass, 0, sizeof(WndClass));
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hbrBackground = NULL; //(HBRUSH)GetStockObject(WHITE_BRUSH);
    WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    //WndClass.hIcon = LoadIcon(hInstance, NULL);
    WndClass.hInstance = 0; //hInstance;
    WndClass.lpfnWndProc = WndProc;
    WndClass.lpszClassName = CLASSNAME;
    WndClass.style = CS_HREDRAW | CS_VREDRAW;
    if(!RegisterClass(&WndClass))
        return -1;

    hWnd = CreateWindow(CLASSNAME, WINNAME, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, VSCREEN_WIDTH, VSCREEN_HEIGHT,
                        NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, SW_SHOWNORMAL); //SW_MAXIMIZE ); //nCmdShow);
    return 0;
}
#endif



//int WINAPI
void    pvm_win_window_thread()
{
    //MSG Message;

    if(win_scr_setup_window())
    {
        init_err = 1;
        printf("pvm_win_setup_window failed\n");
        return;
    }

    win_scr_init_window();

    /*
    // Allocate enough memory for the BITMAPINFOHEADER and 256 RGBQUAD palette entries
    LPBITMAPINFO lpbi;
    lpbi = (LPBITMAPINFO) malloc(sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));

    lpbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    lpbi->bmiHeader.biWidth = VSCREEN_WIDTH;
    lpbi->bmiHeader.biHeight = VSCREEN_HEIGHT;
    lpbi->bmiHeader.biPlanes = 1;
    lpbi->bmiHeader.biBitCount = 24;
    lpbi->bmiHeader.biCompression = BI_RGB;
    lpbi->bmiHeader.biSizeImage = 0;
    lpbi->bmiHeader.biXPelsPerMeter = 0;
    lpbi->bmiHeader.biYPelsPerMeter = 0;
    lpbi->bmiHeader.biClrUsed = 0;
    lpbi->bmiHeader.biClrImportant = 0;

    HDC hScreenDC = GetWindowDC(NULL);

    screenBitmap = CreateDIBSection( hScreenDC, lpbi, DIB_RGB_COLORS, (void*)&win_src_screen_image, 0, 0);

    // Assume hPaintDC is a variable of type HDC, and the dc we're rendering to
    //hBitmapDC = CreateCompatibleDC(hScreenDC);
    //HBITMAP hOldBitmap = (HBITMAP)SelectObject(hBitmapDC, screenBitmap);


    ReleaseDC(NULL,hScreenDC);
    free(lpbi);
    */

    int i;
    for( i = 0; i < VSCREEN_WIDTH * VSCREEN_HEIGHT * 3; i++)
    {
        win_src_screen_image[i] = 34;
    }

    drv_video_win32.screen = win_src_screen_image;
    drv_video_win32.xsize = VSCREEN_WIDTH;
    drv_video_win32.ysize = VSCREEN_HEIGHT;
    drv_video_win32.update = &win_scr_screen_update;
#if 1
    drv_video_win32.bitblt = &vid_bitblt_forw;
#if VIDEO_DRV_WINBLT
    drv_video_win32.winblt = &vid_win_winblt;
#endif
    drv_video_win32.readblt = &vid_readblt_forw;
    drv_video_win32.bitblt_part = &vid_bitblt_part_forw;
#else
    drv_video_win32.bitblt = &drv_video_bitblt_rev;
    drv_video_win32.winblt = &drv_video_win_winblt_rev;
    drv_video_win32.readblt = &drv_video_readblt_rev;
    drv_video_win32.bitblt_part = &drv_video_bitblt_part_rev;
#endif

    drv_video_win32.mouse_redraw_cursor = &vid_mouse_draw_deflt;
    drv_video_win32.mouse_set_cursor = &vid_mouse_set_cursor_deflt;
    drv_video_win32.mouse_disable = &vid_mouse_off_deflt;
    drv_video_win32.mouse_enable = &vid_mouse_on_deflt;

    init_ok = 1;

#if HOVER
    {
        eventTrack.cbSize = sizeof(eventTrack);
        eventTrack.dwFlags = TME_HOVER;
        eventTrack.hwndTrack = hWnd;
        eventTrack.dwHoverTime = 5;

        if(0 == TrackMouseEvent(&eventTrack))
            printf("Track error\n");
    }
#endif

    win_scr_event_loop();
    //return Message.wParam;
    //printf("Message loop end\n");

}


void win_scr_mk_mouse_event(int wParam, int xPos, int yPos )
{
    drv_video_win32.mouse_x = xPos;
    drv_video_win32.mouse_y = yPos;
    drv_video_win32.mouse_flags = wParam;
    drv_video_win32.mouse();

    struct ui_event e;
    e.type = UI_EVENT_TYPE_MOUSE;
    e.time = fast_time();
    e.focus= 0;

    e.m.buttons = wParam;
    e.abs_x = xPos;
    e.abs_y = VSCREEN_HEIGHT - yPos - 1;

    ev_q_put_any( &e );
    //printf("-ms-");            printf("%d,%d\n", xPos, yPos );
}


int pvm_video_init()
{
    video_drv = &drv_video_win32;

    drv_video_win32.screen = 0; // Not ready yet

    printf("Starting windows graphics 'driver'\n" );


    if( win_src_make_thread((void *)&pvm_win_window_thread) )
        panic("can't start window thread");


    int repeat = 10000;
    while(repeat-- > 0)
    {
        //Sleep(20);
        hal_sleep_msec( 20 );
        if( init_err ) break;
        if( init_ok )
        {
            scr_zbuf_init();
            scr_zbuf_turn_upside(1);
            return 0;
        }

    }


    return -1;
}



#endif // __linux__


