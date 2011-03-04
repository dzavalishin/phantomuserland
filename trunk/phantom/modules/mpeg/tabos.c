//#include <gfx/gfx.h>

#include <stdio.h>

#include "tabos_compat.h"

#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480

struct sBitmap *psDesktop = 0;
struct sBitmap *psImage = 0;
unsigned int nWidth = 0, nHeight = 0;
unsigned int nPosX = 0, nPosY = 0;

void spoon_putpixel( int x, int y, unsigned int col ) {
    unsigned short *pRam;

    if ( psImage ) {
        //pRam = ( unsigned short * ) psImage -> pnData;
        //pRam[ y * nWidth + x ] = COL_TO_RGB16( Rgb32ToCol( col ) );
    }
}

void spoon_sync() {
    //BlitBitmap( psDesktop, psImage, MakeRect( 0, 0, nWidth, nHeight ), MakePoint( nPosX, nPosY ), DM_COPY );
}

// -----------------------------------

int init_spoon( int width, int height ) {
    Print( "%dx%d\n", width, height );
    nWidth = width;
    nHeight = height;
/*
    if ( InitGraphics() ) {
        Print( "Could not init graphics\n" );
        return -1;
    }

    if ( SetMode( SCREEN_WIDTH, SCREEN_HEIGHT, CS_RGB16, 60.0f ) ) {
        Print( "Could not set graphics mode\n" );
        return -1;
    }

    psDesktop = GetGfxScreen();
    if ( psDesktop == NULL ) {
        Print( "Could not get graphics screen\n" );
        return -1;
    }
    psImage = CreateBitmap( width, height, CS_RGB16, NULL, 0 );
    if ( psImage == NULL ) {
        Print( "Could not create image bitmap\n" );
        return -1;
    }

    if ( width < SCREEN_WIDTH ) {
        nPosX =  ( SCREEN_WIDTH - width ) / 2;
    }
    if ( height < SCREEN_HEIGHT ) {
        nPosY =  ( SCREEN_HEIGHT - height ) / 2;
    }
*/
    return 0;
}

void clean_spoon() {
    //CloseGraphics();
}
