#include "unix/fat32/define.h"
#include "FAT32_Definitions.h"
#include "FAT32_Opts.h"

#ifndef __FAT32_WRITE_H__
#define __FAT32_WRITE_H__

//-----------------------------------------------------------------------------
//  Globals
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
BOOL FAT32_AddFileEntry(f32_t *impl, UINT32 dirCluster, char *filename, char *shortfilename, UINT32 startCluster, UINT32 size);
BOOL FAT32_AddFreeSpaceToChain(f32_t *impl, UINT32 *startCluster);
BOOL FAT32_AllocateFreeSpace(f32_t *impl, BOOL newFile, UINT32 *startCluster, UINT32 size);

#endif
