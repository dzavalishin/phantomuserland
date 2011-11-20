/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _STAGE2_VESA_H
#define _STAGE2_VESA_H

#ifndef ARCH_ia32
#warning Intel32 code! Wrong arch?
#endif

#include <phantom_types.h>

struct VBEInfoBlock {
    char   	signature[4]; // should be 'VESA'
    u_int16_t version;
    u_int32_t oem_ptr;
    u_int32_t capabilities;
    u_int32_t video_ptr;
    u_int16_t total_memory;
    // VESA 2.x stuff
    u_int16_t oem_software_rev;
    u_int32_t oem_vendor_name_ptr;
    u_int32_t oem_product_name_ptr;
    u_int32_t oem_product_rev_ptr;
    u_int8_t  reserved[222];
    u_int8_t  oem_data[256];
} __attribute__ ((packed));

struct VBEModeInfoBlock {
    u_int16_t attributes;
    u_int8_t  wina_attributes;
    u_int8_t  winb_attributes;
    u_int16_t win_granulatiry;
    u_int16_t win_size;
    u_int16_t wina_segment;
    u_int16_t winb_segment;
    u_int32_t win_function_ptr;
    u_int16_t bytes_per_scanline;

    u_int16_t x_resolution;
    u_int16_t y_resolution;
    u_int8_t  x_charsize;
    u_int8_t  y_charsize;
    u_int8_t  num_planes;
    u_int8_t  bits_per_pixel;
    u_int8_t  num_banks;
    u_int8_t  memory_model;
    u_int8_t  bank_size;
    u_int8_t  num_image_pages;
    u_int8_t  _reserved;

    u_int8_t  red_mask_size;
    u_int8_t  red_field_position;
    u_int8_t  green_mask_size;
    u_int8_t  green_field_position;
    u_int8_t  blue_mask_size;
    u_int8_t  blue_field_position;
    u_int8_t  reserved_mask_size;
    u_int8_t  reserved_field_position;
    u_int8_t  direct_color_mode_info;

    u_int32_t phys_base_ptr;
    u_int32_t offscreen_mem_offset;
    u_int16_t offscreen_mem_size;
    u_int8_t  _reserved2[206];
} __attribute__ ((packed));


#define VBE_MODE_LINEAR 0x4000

#endif

