/*
** Copyright 2002-2003, Michael Noisternig. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _VESA3_H_
#define _VESA3_H_

//#include <kernel/kernel.h>

#define VESA_MAGIC ('P'|('M'<<8)|('I'<<16)|('D'<<24))  // 'PMID'

#define VESA_CODE_SIZE 0x10000
#define VESA_STACK_SIZE 0x2000
#define VESA_DATA_SIZE 0x600

typedef struct vesa3_pm_infoblock {
	uint32		signature;				// PM Info Block signature ('PMID')
	uint16		call_offs;				// offset of PM entry point within BIOS
	uint16		init_offs;				// offset of PM initialization entry point
	uint16		sel_bios_data;			// selector to BIOS data area emulation block
	uint16		sel_a0000;				// selector to 0xa0000
	uint16		sel_b0000;				// selector to 0xb0000
	uint16		sel_b8000;				// selector to 0xb8000
	uint16		sel_code;				// selector to access code segment as data
	uint8		in_pm;					// true iff in protected mode
	uint8		checksum;				// sum of all bytes in this struct must match 0
} _PACKED vesa3_pm_infoblock;

typedef struct vesa3_infoblock {
	uint32		signature;				// VBE Info Block signature ('VESA')
	uint16		version;				// VBE version (0x300)
	uint32		oem_string_ptr;			// OEM string
	uint8		capabilities[4];		// capabilities of graphics controller
	uint32		video_mode_ptr;			// video mode pointer
	uint16		total_memory;			// # 64kb memory blocks
	uint16		oem_software_rev;		// VBE implementation software revision
	uint32		oem_vendor_name_ptr;	// vendor name
	uint32		oem_product_name_ptr;	// product name
	uint32		oem_product_rev_ptr;	// product revision
	uint8		_reserved[222];			// VBE implementation scratch area
	uint8		oem_data[256];			// data area for OEM strings
} _PACKED vesa3_infoblock;

typedef struct vesa3_mode_infoblock {
	
	// mandatory information for all VBE revisions
	uint16		mode_attributes;			// mode attributes
	uint8		win_A_attributes;			// window A attributes
	uint8		win_B_attributes;			// window B attributes
	uint16		win_granularity;			// window granularity
	uint16		win_size;					// window size
	uint16		win_A_segment;				// window A start segment
	uint16		win_B_segment;				// window B start segment
	uint32		win_func_ptr;				// real mode pointer to window function
	uint16		bytes_per_scanline;			// bytes per scan line
	
	// mandatory information for VBE 1.2 and above
	uint16		x_resolution;				// horizontal resolution in pixels (characters)
	uint16		y_resolution;				// vertical resolution in pixels (characters)
	uint8		x_char_size;				// character cell width in pixels
	uint8		y_char_size;				// character cell height in pixels
	uint8		nr_planes;					// number of memory planes
	uint8		bits_per_pixel;				// bits per pixel
	uint8		nr_banks;					// number of banks
	uint8		memory_model;				// memory model type
	uint8		bank_size;					// bank size in KB
	uint8		nr_image_pages;				// number of images
	uint8		_reserved1;					// reserved for page function
	
	// direct color fields (required for direct/6 and YUV/7 memory models)
	uint8		red_mask_size;				// size of direct color red mask in bits
	uint8		red_field_pos;				// bit posision of LSB of red mask
	uint8		green_mask_size;			// size of direct color green mask in bits
	uint8		green_field_pos;			// bit posision of LSB of green mask
	uint8		blue_mask_size;				// size of direct color blue mask in bits
	uint8		blue_field_pos;				// bit posision of LSB of blue mask
	uint8		rsvd_mask_size;				// size of direct color reserved mask in bits
	uint8		rsvd_field_pos;				// bit posision of LSB of reserved mask
	uint8		direct_color_mode_info;		// direct color mode attributes
	
	// mandatory information for VBE 2.0 and above
	uint32		phys_base_ptr;				// physical address for flat memory frame buffer
	uint32		_reserved2;					// 0
	uint16		_reserved3;					// 0
	
	// mandatory information for VBE 3.0 and above
	uint16		linear_bytes_per_scanline;	// linear modes: bytes per scanline
	uint8		banked_nr_image_pages;		// banked modes: number of images
	uint8		linear_nr_image_pages;		// linear modes: number of images
	uint8		linear_red_mask_size;		// linear modes: size of direct color red mask
	uint8		linear_red_field_pos;		// linear modes: bit position of LSB of red mask
	uint8		linear_green_mask_size;		// linear modes: size of direct color green mask
	uint8		linear_green_field_pos;		// linear modes: bit position of LSB of green mask
	uint8		linear_blue_mask_size;		// linear modes: size of direct color blue mask
	uint8		linear_blue_field_pos;		// linear modes: bit position of LSB of blue mask
	uint8		linear_rsvd_mask_size;		// linear modes: size of direct color rsvd mask
	uint8		linear_rsvd_field_pos;		// linear modes: bit position of LSB of rsvd mask
	uint32		max_pixel_clock;			// maximum pixel clock (in Hz) for graphics mode
	
	uint8		_reserved4[189+1];
} _PACKED vesa3_mode_infoblock;

enum vesa3_CRTC_flags {
	VESA3_CRTC_DOUBLE_SCAN = 1,
	VESA3_CRTC_INTERLACED = 2,
	VESA3_CRTC_HSYNC_NEGATIVE = 4,
	VESA3_CRTC_VSYNC_NEGATIVE = 8
};

/* For an explanation of what values to fill in this structure
 * I've pasted some comments from Andy Ritger's gtf.c here:
 *
 * --- gtf.c --->
 *
 * The GTF EXCEL(TM) SPREADSHEET, a sample (and the definitive)
 * implementation of the GTF Timing Standard, is available at:
 *
 * ftp://ftp.vesa.org/pub/GTF/GTF_V1R1.xls
 *
 * Timing description:
 *
 *     <--------1--------> <--2--> <--3--> <--4-->
 *                                _________
 *    |-------------------|_______|       |_______
 *
 *                        R       SS      SE     FL
 *       
 * 1: visible image
 * 2: blank before sync (aka front porch)
 * 3: sync pulse
 * 4: blank after sync (aka back porch)
 * R: Resolution
 * SS: Sync Start
 * SE: Sync End
 * FL: Frame Length
 *
 * <-------------
 *
 * So the h/v values are computed as follows:
 *
 * sync_start = visible pixels + front porch
 * sync_end = sync_start + sync pulse
 * total = sync_end + back porch
 */
typedef struct vesa3_CRTC_infoblock {
	uint16		horizontal_total;			// horizontal total in pixels
	uint16		horizontal_sync_start;		// horizontal sync start in pixels
	uint16		horizontal_sync_end;		// horizontal sync end in pixels
	uint16		vertical_total;				// vertical total in lines
	uint16		vertical_sync_start;		// vertical sync start in lines
	uint16		vertical_sync_end;			// vertical sync end in lines
	uint8		flags;						// flags (double-scan, interlaced, h/v-sync polarity)
	uint32		pixel_clock;				// pixel clock in units of Hz
	uint16		refresh_rate;				// refresh rate in units of 0.01 Hz
	uint8		_reserved[40];
} _PACKED vesa3_CRTC_infoblock;

#endif
