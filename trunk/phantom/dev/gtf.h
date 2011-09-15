/*
 * Copyright 2002-2003, Michael Noisternig. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _GTF_H_
#define _GTF_H_

//#include "vesa3.h"
#include "ia32/vesa3/vesa3.h"

typedef int fixed;  // fixed point format type in 22:10 format

void gtf_compute_CRTC_data( struct vesa3_CRTC_infoblock *crtc,
	int h_pixels, int v_lines, fixed freq, bool interlaced, bool margins );

#endif
