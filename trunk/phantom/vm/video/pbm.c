#if 0
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/

#include <vm/internal_da.h>
#include <vm/object.h>
//#include <stdlib.h>
#include <phantom_libc.h>

#include <video/bitmap.h>

//static inline int isws(unsigned char c)
static inline int isws(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

//static void skip_ws( unsigned char** cpp )
static void skip_ws( char** cpp )
{
    while( isws( **cpp ) )
        (*cpp)++;
}

//static void skip_num( unsigned char** cpp )
static void skip_num( char** cpp )
{
    while( **cpp >= '0' && **cpp <= '9' )
        (*cpp)++;
}

//static void skip_comment( unsigned char** cpp )
static void skip_comment( char** cpp )
{
    if(**cpp != '#') return;
    (*cpp)++;

    while( **cpp != '\r' && **cpp != '\n' )
        (*cpp)++;

    skip_ws( cpp );
}


static void moveImage( rgba_t *to, unsigned char *from, int height, int width, int twobytes )
{
    int row;
    for(row = height - 1;row >= 0;row--){
        unsigned char *rowScan = from + (row * width * (twobytes ? 6 : 3));
        int pixelsLeft;
        for(pixelsLeft = width;pixelsLeft > 0;pixelsLeft--){
            to->a = 255;
            to->r = *rowScan++;
            if(twobytes)
                rowScan++;

            to->g = *rowScan++;
            if(twobytes)
                rowScan++;

            to->b = *rowScan++;
            if(twobytes)
                rowScan++;

            to++;
        }
    }

}


static int parseHeader( unsigned char **pfrom, int *width, int *height, int *maxcolorval)
{
    //unsigned char *from = *pfrom;
    char *from = (char *)*pfrom;

    if( *from++ != 'P' ) return 1;
    if( *from++ != '6' ) return 2;

    skip_ws( &from ); skip_comment( &from );

    if( 1 != sscanf(from, "%d", width ) )
        return 3;

    skip_num( &from ); skip_ws( &from ); skip_comment( &from );

    if( 1 != sscanf(from, "%d", height ) )
        return 4;

    skip_num( &from ); skip_ws( &from ); skip_comment( &from );

    if( 1 != sscanf(from, "%d", maxcolorval ) )
        return 5;

    skip_num( &from );

    if(!isws( *from ) ) return 6;
    from++;

    *pfrom = (unsigned char *)from;

    return 0;
}

// Returns nonzero on failure
int drv_video_ppm_load( drv_video_bitmap_t **to, void *_from )
{
    unsigned char *from = _from;
    int width, height, maxcolorval;

    int rc = parseHeader( &from, &width, &height, &maxcolorval);
    if(rc) return rc;


    *to = malloc(drv_video_bitmap_bytes( width, height ));

    if( *to == NULL ) return 5;

    drv_video_bitmap_t *bmp = *to;

    bmp->xsize = width;
    bmp->ysize = height;

    int twobytes = maxcolorval > 255;

    //rgba_t *pixel = bmp->pixel;
    moveImage( bmp->pixel, from, height, width, twobytes);
    return 0;
}


int drv_video_string2bmp( struct data_area_4_bitmap *bmp, void *_from )
{
    unsigned char *from = _from;
    int width, height, maxcolorval;

    int rc = parseHeader( &from, &width, &height, &maxcolorval);
    if(rc) return rc;


    bmp->image = pvm_create_binary_object(drv_video_bitmap_bytes( width, height ), NULL);

    struct data_area_4_binary *bda = pvm_object_da( bmp->image, binary );

    bmp->xsize = width;
    bmp->ysize = height;

    //struct rgba_t *pixel = bmp->pixel;
    moveImage( (rgba_t *)(bda->data), from, height, width, maxcolorval > 255 );
    return 0;
}


#endif
