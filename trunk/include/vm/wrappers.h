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


#ifndef DRV_VIDEO_WRAPPERS_H_
#define DRV_VIDEO_WRAPPERS_H_

#include "vm/object.h"


int drv_video_bmpblt( struct pvm_object bmp, int xpos, int ypos );



#endif /* DRV_VIDEO_WRAPPERS_H_ */
