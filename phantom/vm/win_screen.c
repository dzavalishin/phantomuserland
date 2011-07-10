/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: no
 * Preliminary: yes
 *
 * This source file implements Windows based wrapper for VM to
 * run in Windows-hosted environment.
 *
 * TODO: GBR -> RGB!!
 *
**/

#ifndef __linux__

#include <phantom_libc.h>

#include <windows.h>
#include <string.h>
#include <assert.h>

#include <drv_video_screen.h>
#include <video/screen.h>

#include "gcc_replacements.h"

#include "winhal.h"

#define VSCREEN_WIDTH 1024
#define VSCREEN_HEIGHT 768


struct drv_video_screen_t        drv_video_win32 =
{
    "Windows",
    // size
    0, 0, 24,
    // mouse x y flags
    0, 0, 0,

    // screen
    0,

probe: (void *)drv_video_null,
start: (void *)drv_video_null,
stop:  (void *)drv_video_null,

    (void*)drv_video_null,
    (void*)drv_video_null,
    (void*)drv_video_null,

    (void*)drv_video_null,

mouse:    		(void*)drv_video_null,

redraw_mouse_cursor: 	drv_video_draw_mouse_deflt,
set_mouse_cursor: 	drv_video_set_mouse_cursor_deflt,
mouse_disable:          drv_video_mouse_off_deflt,
mouse_enable:          	drv_video_mouse_on_deflt,

};




const char *CLASSNAME = "Phantom", *WINNAME = "Phantom test environment";

LRESULT CALLBACK WndProc(HWND hWnd, unsigned int iMessage, WPARAM wParam, LPARAM lParam);

static HWND hWnd;

TRACKMOUSEEVENT eventTrack;


static char * screen_image;
static HBITMAP screenBitmap;

HDC hBitmapDC;

static int eline = 0;


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
        //drv_win_screen_update();
        drv_video_update();
    }
}


void pvm_win_draw_cursor(int x, int y)
{
}




//  -----------------------------------------

#endif



//static
void drv_win_screen_update(void)
{
    GdiFlush();

    eline = 1;
    RedrawWindow( hWnd, 0, 0, RDW_INVALIDATE/*|RDW_NOERASE*/ );

}

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

    case WM_MOUSEMOVE:
        {
            int xPos = (short)(0x0FFFF & lParam);//GET_X_LPARAM(lParam);
            int yPos = VSCREEN_HEIGHT - (short)(0x0FFFF & (lParam>>16));//GET_Y_LPARAM(lParam);

            //	printf("%d,%d\n", xPos, yPos );

            drv_video_win32.mouse_x = xPos;
            drv_video_win32.mouse_y = yPos;
            drv_video_win32.mouse_flags = wParam;
            drv_video_win32.mouse();

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



//int WINAPI
void    pvm_win_window_thread()
{
    MSG Message;

    if(pvm_win_setup_window())
    {
        init_err = 1;
        printf("pvm_win_setup_window failed\n");
        return;
    }

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

    screenBitmap = CreateDIBSection( hScreenDC, lpbi, DIB_RGB_COLORS, (void*)&screen_image, 0, 0);

    // Assume hPaintDC is a variable of type HDC, and the dc we're rendering to
    //hBitmapDC = CreateCompatibleDC(hScreenDC);
    //HBITMAP hOldBitmap = (HBITMAP)SelectObject(hBitmapDC, screenBitmap);


    ReleaseDC(NULL,hScreenDC);
    free(lpbi);


    int i;
    for( i = 0; i < VSCREEN_WIDTH * VSCREEN_HEIGHT * 3; i++)
    {
        screen_image[i] = 34;
    }

    drv_video_win32.screen = screen_image;
    drv_video_win32.xsize = VSCREEN_WIDTH;
    drv_video_win32.ysize = VSCREEN_HEIGHT;
    drv_video_win32.update = &drv_win_screen_update;
#if 1
    drv_video_win32.bitblt = &drv_video_bitblt_forw;
    drv_video_win32.winblt = &drv_video_win_winblt;
    drv_video_win32.readblt = &drv_video_readblt_forw;
#else
    drv_video_win32.bitblt = &drv_video_bitblt_rev;
    drv_video_win32.winblt = &drv_video_win_winblt_rev;
    drv_video_win32.readblt = &drv_video_readblt_rev;
#endif

    drv_video_win32.redraw_mouse_cursor = &drv_video_draw_mouse_deflt;
    drv_video_win32.set_mouse_cursor = &drv_video_set_mouse_cursor_deflt;
    drv_video_win32.mouse_disable = &drv_video_mouse_off_deflt;
    drv_video_win32.mouse_enable = &drv_video_mouse_on_deflt;

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

    while(GetMessage(&Message, hWnd, 0, 0))
    {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }
    //return Message.wParam;
    //printf("Message loop end\n");

}


int pvm_win_init()
{
    drv_video_win32.screen = 0; // Not ready yet

    printf("Starting windows graphics 'driver'\n" );


    static unsigned long tid;
    if( 0 == CreateThread( 0, 0, (void *)&pvm_win_window_thread, (void*)0, 0, &tid ) )
        panic("can't start window thread");


    int repeat = 10000;
    while(repeat-- > 0)
    {
        Sleep(20);
        if( init_err ) break;
        if( init_ok )
        {
//#if VIDEO_ZBUF
            video_zbuf_init();
            video_zbuf_turn_upside(1);
//#endif
            return 0;
        }

    }


    return -1;
}



#endif // __linux__


