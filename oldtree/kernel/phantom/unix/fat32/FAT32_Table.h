#include "unix/fat32/define.h"
#include "FAT32_Opts.h"

#ifndef __FAT32_TABLE_H__
#define __FAT32_TABLE_H__


//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
void FAT32_InitFatBuffer(f32_t *impl );
BOOL FAT32_ReadFATSector(f32_t *impl, UINT32 sector);
BOOL FAT32_WriteFATSector(f32_t *impl, UINT32 sector);
BOOL FAT32_PurgeFATBuffer(f32_t *impl);
UINT32 FAT32_FindNextCluster(f32_t *impl, UINT32 Current_Cluster);
UINT32 FAT32_GetFsInfoNextCluster(f32_t *impl);
void FAT32_SetFsInfoNextCluster(f32_t *impl, UINT32 newValue);
BOOL FAT32_FindBlankCluster(f32_t *impl, UINT32 StartCluster, UINT32 *FreeCluster);
BOOL FAT32_SetClusterValue(f32_t *impl, UINT32 Cluster, UINT32 NextCluster);
BOOL FAT32_AddClusterToEndofChain(f32_t *impl, UINT32 StartCluster, UINT32 newEntry);
BOOL FAT32_FreeClusterChain(f32_t *impl, UINT32 StartCluster);

#endif
