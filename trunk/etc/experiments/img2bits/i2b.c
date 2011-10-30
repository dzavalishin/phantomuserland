#include <stdio.h>
#include <assert.h>
#include "c.h"

//#def

#define ALPHA_BITS_RIGHT 0

#if 0
static void alpha_to_bits_line( u_int32_t *bits, size_t ndwords, rgba_t *pixels, size_t npixels )
{
    printf( "nwdords %2d npix %3d\t", ndwords, npixels );

    int bitsLeft = 32;
    *bits = 0;
    while( (npixels-- > 0) && (ndwords > 0) )
    {
        if( bitsLeft <= 0 )
        {
            bits++;
            *bits = 0;
            bitsLeft = 32;
            ndwords--;
        }
#if ALPHA_BITS_RIGHT
        *bits >>= 1;

        if( pixels->a )
            *bits |= 0x80000000;
#else
        *bits <<= 1;

        if( pixels->a )
            *bits |= 1;
#endif
        pixels++;
        bitsLeft--;
    }
}

static void dump_bits( u_int32_t *bits, size_t ndwords )
{
    while( ndwords-- > 0 )
    {
        int bitsLeft = 32;
        while( bitsLeft-- > 0 )
        {
            putchar( (*bits & 0x80000000) ? '*' : '.' );
            *bits <<= 1;
        }
        bits++;
    }

    //putchar( '\n' );
    printf( "\n" );
}

static void alpha_to_bits( u_int32_t *bits, size_t ndwords, rgba_t *pixels, int xsize, int ysize )
{
    assert(xsize > 0);
    //assert(ysize > 0);

    int npixels = xsize * ysize;
    size_t skip_dwords = ((xsize-1)/32)+1;

    printf( "%d x %d, skip %d, npix %d\n", xsize, ysize, skip_dwords, npixels );

    while( ysize-- > 0 )
    {
        alpha_to_bits_line( bits, skip_dwords, pixels, xsize );
        dump_bits( bits, skip_dwords );
        bits += skip_dwords;
        pixels += xsize;
        npixels -= xsize;
        ndwords -= skip_dwords;
    }
}
#endif

static void alpha_to_bits_line( u_int8_t *bits, size_t noutbytes, rgba_t *pixels, size_t npixels )
{
    int bitsLeft = 8;
    *bits = 0;
    while( (npixels-- > 0) && (noutbytes > 0) )
    {
        if( bitsLeft <= 0 )
        {
            bits++;
            *bits = 0;
            bitsLeft = 8;
            noutbytes--;
        }
#if ALPHA_BITS_RIGHT
        *bits >>= 1;

        if( pixels->a )
            *bits |= 0x80;
#else
        *bits <<= 1;

        if( pixels->a )
            *bits |= 1;
#endif
        pixels++;
        bitsLeft--;
    }
}

static void dump_bits( u_int8_t *bits, size_t bpl, size_t noutbytes )
{
    int l = 0;
    while( noutbytes-- > 0 )
    {
        int bitsLeft = 8;
        while( bitsLeft-- > 0 )
        {
            putchar( (*bits & 0x80) ? '*' : '.' );
            *bits <<= 1;
        }
        bits++;

    l++;
    if( l >= bpl )
    {
    	putchar( '\n' );
		l = 0;
    }


    }

    printf( "\n" );
}



static void alpha_to_bits( u_int8_t *bits, size_t noutbytes, rgba_t *pixels, int xsize, int ysize )
{
    assert(xsize > 0);
    assert(ysize > 0);

    int npixels = xsize * ysize;
    size_t skip_bytes = ((xsize-1)/8)+1;

    int ycnt = 0;

    //SHOW_FLOW( 0, "%d x %d, skip %d, npix %d", xsize, ysize, skip_bytes, npixels );

    while( ycnt < ysize )
    {
        u_int8_t *bp = bits + (skip_bytes * (ysize-ycnt-1)); // flip y

        alpha_to_bits_line( bp, skip_bytes, pixels, xsize );
        pixels += xsize;
        npixels -= xsize;
        noutbytes -= skip_bytes;
        //bits += skip_bytes;
        ycnt++;
    }

    dump_bits( bits, skip_bytes, skip_bytes*ysize );

}


extern rgba_t mbmp[];

int main()
{
    unsigned int bits[128];
    alpha_to_bits( bits, sizeof(bits), mbmp, 8, 16 );
}
