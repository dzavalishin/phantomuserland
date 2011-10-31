#include "unix/fat32/define.h"
#include "FAT32_Definitions.h"
#include "FAT32_Opts.h"

#ifndef __FAT32_ACCESS_H__
#define __FAT32_ACCESS_H__

//-----------------------------------------------------------------------------
//  Globals
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
BOOL FAT32_InitFAT(f32_t *impl, int rawDisk );
BOOL FAT32_SectorReader(f32_t *impl, UINT32 Startcluster, UINT32 offset);
BOOL FAT32_SectorWriter(f32_t *impl, UINT32 Startcluster, UINT32 offset);
void FAT32_ShowFATDetails(f32_t *impl);
UINT32 FAT32_GetRootCluster(f32_t *impl);
UINT32 FAT32_GetFileEntry(f32_t *impl, UINT32 Cluster, char *nametofind, FAT32_ShortEntry *sfEntry);
BOOL FAT32_SFNexists(f32_t *impl, UINT32 Cluster, char *shortname);
BOOL FAT32_UpdateFileLength(f32_t *impl, UINT32 Cluster, char *shortname, UINT32 fileLength);
BOOL FAT32_MarkFileDeleted(f32_t *impl, UINT32 Cluster, char *shortname);
void ListDirectory(f32_t *impl, UINT32 StartCluster);

#endif
