#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <video/rect.h>


#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480

static int   fb = -1;

//static struct sBitmap *psDesktop = 0;
//static struct sBitmap *psImage = 0;
static unsigned int nWidth = 0, nHeight = 0;
//unsigned int nPosX = 0, nPosY = 0;


//static u_int32_t        *pixels;
static u_int32_t        pixels[1280*1024];



void spoon_putpixel( int x, int y, unsigned int col ) {
    //unsigned short *pRam;

    //if ( pixels )
    {
        //pRam = ( unsigned short * ) psImage -> pnData;
        //pRam[ y * nWidth + x ] = COL_TO_RGB16( Rgb32ToCol( col ) );

        pixels[ y * nWidth + x ] = col;
    }
}

void spoon_sync()
{
    //BlitBitmap( psDesktop, psImage, MakeRect( 0, 0, nWidth, nHeight ), MakePoint( nPosX, nPosY ), DM_COPY );
    write( fb, pixels, sizeof(int)*nWidth*nHeight );
    ioctl( fb, IOCTL_FB_FLUSH, 0, 0 );

}

// -----------------------------------

int init_spoon( int width, int height ) {
    printf( "Open framebuf %dx%d\n", width, height );
    nWidth = width;
    nHeight = height;

    if( (width * height) > (1280*1024) )
    {
        printf("pic too big\n");
        exit(1);
    }

    //pixels = calloc( sizeof(int)*nWidth*nHeight );

    fb = open("/dev/etc/fb0", O_RDWR );

    if( fb < 0 )
    {
        printf("can't open framebuf\n");
        exit(1);
    }

    char a[2000];
    memset( a, 0x6F, sizeof(a) );
    write( fb, a, sizeof(a) );

    rect_t r;
    r.xsize = width;
    r.ysize = height;
    r.x = r.y = 0;

    ioctl( fb, IOCTL_FB_SETBOUNDS, &r, sizeof(r) );

/*
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
