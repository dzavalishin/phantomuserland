/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/


#include <vm/internal_da.h>
//#include <drv_video_screen.h>
#include <video/screen.h>
#include <vm/wrappers.h>


// interfaces

// TODO killme?
// Returns nonzero if class is not bitmap
int drv_video_bmpblt( struct pvm_object _bmp, int xpos, int ypos, int zpos )
{

    if( !pvm_object_class_is( _bmp, pvm_get_bitmap_class() ) )
    	return 1;

    struct data_area_4_bitmap *bmp = pvm_object_da( _bmp, bitmap );
    struct data_area_4_binary *bin = pvm_object_da( bmp->image, binary );

    drv_video_bitblt( (void *)bin->data, 0, 0, bmp->xsize, bmp->ysize, (zbuf_t)zpos, 0 );
    return 0;
}
