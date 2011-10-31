#if HAVE_UNIX

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//					        FAT32 File IO Library
//								    V2.0
// 	  							 Rob Riglar
//						    Copyright 2003 - 2007
//
//   					  Email: rob@robriglar.com
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2003-2007, Rob Riglar - Robs-Projects.com
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY ROBS-PROJECTS.COM ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL ROBS-PROJECTS.COM BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include "FAT32_Definitions.h"
#include "FAT32_Base.h"
#include "FAT32_Table.h"

//-----------------------------------------------------------------------------
//							FAT Sector Buffer
//-----------------------------------------------------------------------------

#define FAT32_GET_32BIT_WORD(location)	( GET_32BIT_WORD(impl->FATBuffer.Data, location) )
#define FAT32_SET_32BIT_WORD(location, value)	{ SET_32BIT_WORD(impl->FATBuffer.Data, location, value); impl->FATBuffer.Changed = TRUE; }

//-----------------------------------------------------------------------------
// FAT32_InitFatBuffer:
//-----------------------------------------------------------------------------
void FAT32_InitFatBuffer(f32_t *impl)
{
    impl->FATBuffer.Sector = 0xFFFFFFFF;
    impl->FATBuffer.Changed = FALSE;
    impl->FATBuffer.Reads = 0;
    impl->FATBuffer.Writes = 0;
    memset(impl->FATBuffer.Data, 0x00, sizeof(impl->FATBuffer.Data));
}
//-----------------------------------------------------------------------------
// FAT32_ReadFATSector: Read a FAT sector
//-----------------------------------------------------------------------------
BOOL FAT32_ReadFATSector(f32_t *impl, UINT32 sector)
{
    // Only do anything if sector not already loaded
    if ( (sector!=impl->FATBuffer.Sector) )
    {
        // Writeback
        if (impl->FATBuffer.Changed)
        {
            impl->FATBuffer.Writes++;
            if (!impl->FAT_WriteSector(impl, impl->FATBuffer.Sector, impl->FATBuffer.Data))
                return FALSE;
        }

        impl->FATBuffer.Sector = sector;
        impl->FATBuffer.Changed = FALSE;

        // Read next sector
        if (!impl->FAT_ReadSector(impl, impl->FATBuffer.Sector, impl->FATBuffer.Data))
            return FALSE;

        impl->FATBuffer.Reads++;
    }

    return TRUE;
}
//-----------------------------------------------------------------------------
// FAT32_WriteFATSector: Write a FAT sector
//-----------------------------------------------------------------------------
BOOL FAT32_WriteFATSector(f32_t *impl, UINT32 sector)
{
#ifdef FATBUFFER_IMMEDIATE_WRITEBACK
    impl->FATBuffer.Sector = sector;
    impl->FATBuffer.Changed = FALSE;
    impl->FATBuffer.Writes++;
    return impl->FAT_WriteSector(impl, impl->FATBuffer.Sector, impl->FATBuffer.Data);
#else
    (void) impl;
    return TRUE;
#endif
}
//-----------------------------------------------------------------------------
// FAT32_ReadFATSector: Read a FAT sector
//-----------------------------------------------------------------------------
BOOL FAT32_PurgeFATBuffer( f32_t *impl )
{
#ifndef FATBUFFER_IMMEDIATE_WRITEBACK

    // Writeback
    if (impl->FATBuffer.Changed)
    {
        impl->FATBuffer.Writes++;
        if (!impl->FAT_WriteSector(impl->FATBuffer.Sector, impl->FATBuffer.Data))
            return FALSE;

        impl->FATBuffer.Changed = FALSE;
    }
#else
    (void) impl;
#endif

    return TRUE;
}

//-----------------------------------------------------------------------------
//						General FAT Table Operations
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// FAT32_FindNextCluster: Return Cluster number of next cluster in chain by
// reading FAT table and traversing it. Return 0xffffffff for end of chain.
//-----------------------------------------------------------------------------
UINT32 FAT32_FindNextCluster(f32_t *impl, UINT32 Current_Cluster)
{
    UINT32 FAT_sector_offset, position;
    UINT32 nextcluster;

    // Why is '..' labelled with cluster 0 when it should be 2 ??
    if (Current_Cluster==0) Current_Cluster=2;

    // Find which sector of FAT table to read
    FAT_sector_offset = Current_Cluster / 128;

    // Read FAT sector into buffer
    if (!FAT32_ReadFATSector(impl, impl->FAT32.fat_begin_lba+FAT_sector_offset))
        return (FAT32_EOC_FLAG);

    // Find 32 bit entry of current sector relating to cluster number
    position = (Current_Cluster - (FAT_sector_offset * 128)) * 4;

    // Read Next Clusters value from Sector Buffer
    nextcluster = FAT32_GET_32BIT_WORD((UINT16)position);

    // Mask out MS 4 bits (its 28bit addressing)
    nextcluster = nextcluster & 0x0FFFFFFF;

    // If 0x0FFFFFFF then end of chain found
    if (nextcluster==0x0FFFFFFF)
        return (FAT32_EOC_FLAG);
    else
        // Else return next cluster
        return (nextcluster);
}
//-----------------------------------------------------------------------------
// FAT32_GetFsInfoNextCluster: Read the next free cluster from FS info block
//-----------------------------------------------------------------------------
UINT32 FAT32_GetFsInfoNextCluster(f32_t *impl)
{
    UINT32 nextFreeCluster = 0xFFFFFFFF;

    // Read FSINFO sector into buffer
    FAT32_ReadFATSector(impl, impl->FAT32.lba_begin+impl->FAT32.fs_info_sector);

    // Get next free cluster
    nextFreeCluster = FAT32_GET_32BIT_WORD(492);

    return nextFreeCluster;
}
//-----------------------------------------------------------------------------
// FAT32_SetFsInfoNextCluster: Write the next free cluster to the FSINFO table
//-----------------------------------------------------------------------------
void FAT32_SetFsInfoNextCluster(f32_t *impl, UINT32 newValue)
{
    // Load sector to change it
    if (!FAT32_ReadFATSector(impl, impl->FAT32.lba_begin+impl->FAT32.fs_info_sector))
        return ;

    // Change
    FAT32_SET_32BIT_WORD(492, newValue);

    // Write back
    FAT32_WriteFATSector(impl, impl->FAT32.lba_begin+impl->FAT32.fs_info_sector);
}
//-----------------------------------------------------------------------------
// FAT32_FindBlankCluster: Find a free cluster entry by reading the FAT
//-----------------------------------------------------------------------------
#ifdef INCLUDE_WRITE_SUPPORT
BOOL FAT32_FindBlankCluster(f32_t *impl, UINT32 StartCluster, UINT32 *FreeCluster)
{
    UINT32 FAT_sector_offset, position;
    UINT32 nextcluster;
    UINT32 Current_Cluster = StartCluster;

    do
    {
        // Find which sector of FAT table to read
        FAT_sector_offset = Current_Cluster / 128;

        if ( FAT_sector_offset < impl->FAT32.fat_sectors)
        {
            // Read FAT sector into buffer
            FAT32_ReadFATSector(impl, impl->FAT32.fat_begin_lba+FAT_sector_offset);

            // Find 32 bit entry of current sector relating to cluster number
            position = (Current_Cluster - (FAT_sector_offset * 128)) * 4;

            // Read Next Clusters value from Sector Buffer
            nextcluster = FAT32_GET_32BIT_WORD((UINT16)position);

            // Mask out MS 4 bits (its 28bit addressing)
            nextcluster = nextcluster & 0x0FFFFFFF;

            if (nextcluster !=0 )
                Current_Cluster++;
        }
        else
            // Otherwise, run out of FAT sectors to check...
            return FALSE;
    }
    while (nextcluster != 0x0);

    // Found blank entry
    *FreeCluster = Current_Cluster;
    return TRUE;
}
#endif
//-----------------------------------------------------------------------------
// FAT32_SetClusterValue: Set a cluster link in the chain. NOTE: Immediate
// write (slow).
//-----------------------------------------------------------------------------
#ifdef INCLUDE_WRITE_SUPPORT
BOOL FAT32_SetClusterValue(f32_t *impl, UINT32 Cluster, UINT32 NextCluster)
{
    UINT32 FAT_sector_offset, position;

    // Find which sector of FAT table to read
    FAT_sector_offset = Cluster / 128;

    // Read FAT sector into buffer
    FAT32_ReadFATSector(impl, impl->FAT32.fat_begin_lba+FAT_sector_offset);

    // Find 32 bit entry of current sector relating to cluster number
    position = (Cluster - (FAT_sector_offset * 128)) * 4;

    // Write Next Clusters value to Sector Buffer
    FAT32_SET_32BIT_WORD((UINT16)position, NextCluster);

    // Write FAT sector from buffer
    FAT32_WriteFATSector(impl, impl->FAT32.fat_begin_lba+FAT_sector_offset);

    return TRUE;
}
#endif
//-----------------------------------------------------------------------------
// FAT32_FreeClusterChain: Follow a chain marking each element as free
//-----------------------------------------------------------------------------
#ifdef INCLUDE_WRITE_SUPPORT
BOOL FAT32_FreeClusterChain(f32_t *impl, UINT32 StartCluster)
{
    UINT32 last_cluster;
    UINT32 next_cluster = StartCluster;

    // Loop until end of chain
    while ( (next_cluster!=0xFFFFFFFF) && (next_cluster!=0x00000000) )
    {
        last_cluster = next_cluster;

        // Find next link
        next_cluster = FAT32_FindNextCluster(impl, next_cluster);

        // Clear last link
        FAT32_SetClusterValue(impl, last_cluster, 0x00000000);
    }

    return TRUE;
}
#endif
//-----------------------------------------------------------------------------
// FAT32_AddClusterToEndofChain: Follow a chain marking and then add a new entry
// to the current tail.
//-----------------------------------------------------------------------------
#ifdef INCLUDE_WRITE_SUPPORT
BOOL FAT32_AddClusterToEndofChain(f32_t *impl, UINT32 StartCluster, UINT32 newEntry)
{
    UINT32 last_cluster = 0xFFFFFFFF;
    UINT32 next_cluster = StartCluster;

    if (StartCluster == 0xFFFFFFFF)
        return FALSE;

    // Loop until end of chain
    while ( next_cluster!=0xFFFFFFFF )
    {
        last_cluster = next_cluster;

        // Find next link
        next_cluster = FAT32_FindNextCluster(impl, next_cluster);

        if (next_cluster==0x00000000)
            return FALSE;
    }

    // Add link in for new cluster
    FAT32_SetClusterValue(impl, last_cluster, newEntry);

    // Mark new cluster as end of chain
    FAT32_SetClusterValue(impl, newEntry, 0xFFFFFFFF);

    return TRUE;
}
#endif

#endif // #if HAVE_UNIX
