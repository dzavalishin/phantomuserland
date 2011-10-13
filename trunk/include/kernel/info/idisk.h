/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Disk drive properties.
 *
**/

#ifndef KERNEL_I_DISK_H
#define KERNEL_I_DISK_H

#include <phantom_types.h>

#define I_DISK_MAX_MODEL  40
#define I_DISK_MAX_SERIAL 20

#define I_DISK_HAS_LBA28  (1<<0)
#define I_DISK_HAS_LBA48  (1<<1)
#define I_DISK_HAS_TRIM   (1<<2)
#define I_DISK_HAS_DMA    (1<<3)
#define I_DISK_HAS_SMART  (1<<4)
#define I_DISK_HAS_MULT   (1<<5)
#define I_DISK_HAS_FLUSH  (1<<6)
#define I_DISK_HAS_REMOV  (1<<7)

typedef enum {
    idt_unknown, idt_ata, idt_atapi, idt_cf
} i_disk_type_t;

typedef struct {
    u_int32_t           has;

    i_disk_type_t       type;

    u_int32_t           sectorSize;
    u_int32_t           nSectors;

    u_int32_t           maxMultSectors;

    char 		model[I_DISK_MAX_MODEL+1];
    char                serial[I_DISK_MAX_SERIAL+1];

} i_disk_t;

void dump_i_disk( i_disk_t *info );
void parse_i_disk_ata( i_disk_t *info, u_int16_t p[256] );

#endif // KERNEL_I_DISK_H


