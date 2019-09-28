/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: no, this is not a kernel code.
 *
 * WinAPI specific part.
 * Must use compiler's includes!
 *
 * TODO: GBR -> RGB!!
 *
**/

#ifndef __linux__

#include <windows.h>
#include <stdio.h>
#include "winhal.h"

static const char *CLASSNAME = "Phantom", *WINNAME = "Phantom test environment";

static LRESULT CALLBACK WndProc(HWND hWnd, unsigned int iMessage, WPARAM wParam, LPARAM lParam);

static HWND hWnd;

static TRACKMOUSEEVENT eventTrack;


static HBITMAP screenBitmap;

static HDC hBitmapDC;

//static int eline = 0;

char * win_src_screen_image;



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

#if 1
            win_scr_mk_mouse_event( wParam, xPos, yPos );
#endif
        }
        break;

    default:
        return DefWindowProc(hWnd, iMessage, wParam, lParam);
    }
    return 0;
}






int win_scr_setup_window(void)
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


void win_scr_init_window(void)
{
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
}



void win_scr_event_loop(void)
{
    MSG Message;

    while(GetMessage(&Message, hWnd, 0, 0))
    {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }
}


void win_scr_screen_update(void)
{
    GdiFlush();

    //eline = 1;
    RedrawWindow( hWnd, 0, 0, RDW_INVALIDATE/*|RDW_NOERASE*/ );

}

int win_src_make_thread( void *tfunc )
{
    static unsigned long tid;
    return (0 == CreateThread( 0, 0, tfunc, (void*)0, 0, &tid ) );
}


#endif

