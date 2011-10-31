#include <phantom_types.h>

#include "unix/fat32/define.h"
#include "FAT32_Definitions.h"
#include "FAT32_Opts.h"

#ifndef __FATMISC_H__
#define __FATMISC_H__

//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
void FATMisc_ClearLFN(f32_t * impl, u_int8_t wipeTable);
void FATMisc_CacheLFN(f32_t *impl, u_int8_t *entryBuffer);
void FATMisc_GetLFNCache(f32_t *impl, u_int8_t *strOut);
int FATMisc_If_LFN_TextOnly(f32_t *impl, FAT32_ShortEntry *entry);
int FATMisc_If_LFN_Invalid(f32_t *impl, FAT32_ShortEntry *entry);
int FATMisc_If_LFN_Exists(f32_t *impl, FAT32_ShortEntry *entry);
int FATMisc_If_noLFN_SFN_Only(f32_t *impl, FAT32_ShortEntry *entry);
int FATMisc_If_dir_entry(f32_t *impl, FAT32_ShortEntry *entry);
int FATMisc_If_file_entry(f32_t *impl, FAT32_ShortEntry *entry);
int FATMisc_LFN_to_entry_count(char *filename);
void FATMisc_LFN_to_lfn_entry(f32_t *impl, char *filename, u_int8_t *buffer, int entry, u_int8_t sfnChk);
void FATMisc_Create_sfn_entry(f32_t *impl, char *shortfilename, UINT32 size, UINT32 startCluster, FAT32_ShortEntry *entry);
BOOL FATMisc_CreateSFN(f32_t *impl, char *sfn_output, char *filename);
BOOL FATMisc_GenerateTail(f32_t *impl, char *sfn_output, char *sfn_input, UINT32 tailNum);

#endif
