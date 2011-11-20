/*
 * Copyright 2002-2003, Michael Noisternig. All rights reserved.
 * Distributed under the terms of the NewOS License.
 *
 *
 * This program is based on the VESA Coordinating Video Timing Generator spreadsheet
 * which is available at:
 *
 * http://www.vesa.org/public/CVT/CVTd6r1.xls
 */

#include <newos/compat.h>
#include "gtf.h"

#include <kernel/debug.h>
#include <stdio.h>


// Standard Timing default parameters

#define MARGIN_PERCENT		1.8		// % of active image width/height
#define CELL_GRAN			8		// character cell granularity
#define H_SYNC_PERCENT		8.0		// % of line period for nominal hsync
#define MIN_VSYNC_BP		550		// min. time of vsync + back porch (microsec)
#define V_SYNC				4		// # lines for vsync
#define MIN_V_PORCH			3		// min. # lines for vertical front porch
#define MIN_V_BP			6		// min. # lines for vertical back porch

#define CLOCK_STEP			250		// pixel clock step size (kHz)


// Generalized blanking limitation formula constants

#define M					600		// blanking formula gradient
#define C					40		// blanking formula offset
#define K					128		// blanking formula scaling factor
#define J					20		// blanking formula scaling factor weighting

#define M_PRIME				(M * K / 256)
#define C_PRIME				(((C - J) * K / 256) + J)


#define FIXED(x) ((unsigned)((x)*1024))


void gtf_compute_CRTC_data( struct vesa3_CRTC_infoblock *crtc,
	int h_pixels, int v_lines, fixed freq, bool interlaced, bool margins )
{
	unsigned margin_left_right = 0;
	unsigned margin_top_bottom = 0;
	fixed h_period_est;
	unsigned v_sync_plus_bp;
	unsigned v_lines_total;
	unsigned h_pixels_total;
	unsigned ideal_duty_cycle;
	unsigned h_blank;
	unsigned pixel_clock;

	h_pixels = h_pixels / CELL_GRAN * CELL_GRAN;

	interlaced = interlaced ? 1 : 0;

	if ( margins ) {
		margin_left_right = (h_pixels * FIXED(MARGIN_PERCENT) / (FIXED(100) * CELL_GRAN)) * CELL_GRAN;
		margin_top_bottom = v_lines * FIXED(MARGIN_PERCENT) / FIXED(100);
	}

	h_period_est = (2*FIXED(FIXED(1000000)/freq) - 2*FIXED(MIN_VSYNC_BP))
		/ (2*(v_lines+2*margin_top_bottom+MIN_V_PORCH) + interlaced);

	v_sync_plus_bp = FIXED(MIN_VSYNC_BP)/h_period_est + 1;
	if ( v_sync_plus_bp < V_SYNC + MIN_V_BP )
		v_sync_plus_bp = V_SYNC + MIN_V_BP;

	v_lines_total = ((v_lines + 2*margin_top_bottom + v_sync_plus_bp + MIN_V_PORCH)
		<< interlaced) + interlaced;

	h_pixels_total = h_pixels + 2*margin_left_right;

	ideal_duty_cycle = C_PRIME - (M_PRIME * h_period_est / FIXED(1000));
	if ( ideal_duty_cycle < 20 )
		ideal_duty_cycle = 20;

	h_blank = h_pixels_total*ideal_duty_cycle/((100-ideal_duty_cycle)*(2*CELL_GRAN)) * (2*CELL_GRAN);

	h_pixels_total += h_blank;

	pixel_clock = FIXED(h_pixels_total) * (1000/CLOCK_STEP) / h_period_est
		* CLOCK_STEP * 1000;  // in Hz
	//h_freq = pixel_clock / h_pixels_total;
	//v_frame_rate = pixel_clock / (h_pixels_total * v_lines_total);

	crtc->horizontal_total = h_pixels_total;
	crtc->horizontal_sync_end = h_pixels_total - h_blank/2;
	crtc->horizontal_sync_start = crtc->horizontal_sync_end -
		h_pixels_total * FIXED(H_SYNC_PERCENT) / (FIXED(100) * CELL_GRAN) * CELL_GRAN;

	crtc->vertical_total = v_lines_total;
	crtc->vertical_sync_start = v_lines + MIN_V_PORCH;
	crtc->vertical_sync_end = v_lines + MIN_V_PORCH + V_SYNC;

	crtc->flags = VESA3_CRTC_HSYNC_NEGATIVE;
	if ( interlaced )
		crtc->flags |= VESA3_CRTC_INTERLACED;

	crtc->pixel_clock = pixel_clock;
	crtc->refresh_rate = 100 * pixel_clock / (h_pixels_total * v_lines_total);  // in 0.01 Hz

	printf( "vesa3: CRTC values: "
		"horizontal_total: %u, horizontal_sync_start: %u, horizontal_sync_end: %u, "
		"vertical_total: %u, vertical_sync_start: %u, vertical_sync_end: %u, "
		"flags: 0x%x, pixel_clock: %u, refresh_rate: %u\n",
		crtc->horizontal_total, crtc->horizontal_sync_start, crtc->horizontal_sync_end,
		crtc->vertical_total, crtc->vertical_sync_start, crtc->vertical_sync_end,
		crtc->flags, crtc->pixel_clock, crtc->refresh_rate
		);
}
