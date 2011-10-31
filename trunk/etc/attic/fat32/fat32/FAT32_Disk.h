#include "unix/fat32/define.h"
#include "FAT32_Opts.h"

#ifndef __FAT32_DISK_H__
#define __FAT32_DISK_H__

BOOL FAT32_InitDrive(f32_t *impl);
//BOOL FAT_ReadSector(f32_t *impl, UINT32 sector, BYTE *buffer);
//BOOL FAT_WriteSector(f32_t *impl, UINT32 sector, BYTE *buffer);

#endif
