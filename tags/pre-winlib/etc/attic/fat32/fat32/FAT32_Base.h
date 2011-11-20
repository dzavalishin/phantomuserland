#include "unix/fat32/define.h"
#include "FAT32_Opts.h"

#ifndef __FAT32_BASE_H__
#define __FAT32_BASE_H__

#include "FAT32_Disk.h"

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
BOOL FAT32_FindLBABegin(f32_t *impl, BYTE *buffer, UINT32 *lbaBegin);
UINT32 FAT32_LBAofCluster(f32_t *impl, UINT32 Cluster_Number);
BOOL FAT32_Init(f32_t *impl, int rawDisk  );

#endif
