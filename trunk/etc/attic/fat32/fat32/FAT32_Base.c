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
// FAT32_FindLBABegin: This function is used to find the LBA Address of the first
// volume on the disc. Also checks are performed on the signature and identity
// codes to make sure the partition is FAT32.
//-----------------------------------------------------------------------------
BOOL FAT32_FindLBABegin(f32_t *impl, BYTE *buffer, UINT32 *lbaBegin)
{
    if (buffer==NULL)
        return FALSE;

    // Load MBR (LBA 0) into the 512 byte buffer
    if (!impl->FAT_ReadSector(impl, 0, buffer))
        return FALSE;

    // Make Sure 0x55 and 0xAA are at end of sector
    if (GET_16BIT_WORD(buffer, Signature_Position)!=Signature_Value)
        return FALSE;

    // TODO: Verify this

    // Check the partition type code
    switch(buffer[PARTITION1_TYPECODE_LOCATION])
    {
    case 0x0B: break;
    case 0x06: break;
    case 0x0C: break;
    case 0x0E: break;
    case 0x0F: break;
    case 0x05: break;
    default:
        if (buffer[PARTITION1_TYPECODE_LOCATION] > 0x06)
            return FALSE;
        break;
    }

    // Read LBA Begin for FAT32 File system is located for partition
    *lbaBegin=GET_32BIT_WORD(buffer, PARTITION1_LBA_BEGIN_LOCATION);

    // Return the LBA address of FAT table
    return TRUE;
}
//-----------------------------------------------------------------------------
// FAT32_LBAofCluster: This function converts a cluster number into a sector /
// LBA number.
//-----------------------------------------------------------------------------
UINT32 FAT32_LBAofCluster(f32_t *impl, UINT32 Cluster_Number)
{
    return ((impl->FAT32.cluster_begin_lba + ((Cluster_Number-2)*impl->FAT32.SectorsPerCluster)));
}
//-----------------------------------------------------------------------------
// FAT32_Init: Uses FAT32_FindLBABegin to find the LBA for the volume,
// and loads into memory some specific details of the partitionw which are used
// in further calculations.
//-----------------------------------------------------------------------------
BOOL FAT32_Init(f32_t *impl, int rawDisk )
{
    BYTE buffer[512];

    BYTE Number_of_FATS;
    UINT16 Reserved_Sectors;
    UINT32 LbaBegin;
    UINT32 FATSz;
    UINT32 RootDirSectors;
    UINT32 TotSec;
    UINT32 DataSec;
    UINT32 CountofClusters;

    FAT32_InitFatBuffer(impl);

    if( rawDisk )
        LbaBegin = 0;
    else
    {
        // Check Volume 1 and find LBA address
        if( !FAT32_FindLBABegin( impl, buffer, &LbaBegin) )
            return FALSE;
    }

    impl->FAT32.lba_begin = LbaBegin;

    // Load Volume 1 table into sector buffer
    if (!impl->FAT_ReadSector( impl, LbaBegin, buffer))
        return FALSE;

    // Make sure there are 512 bytes per cluster
    if (GET_16BIT_WORD(buffer, 0x0B)!=0x200)
        return FALSE;

    // Load Parameters of FAT32
    impl->FAT32.SectorsPerCluster = buffer[BPB_SecPerClus];
    Reserved_Sectors = GET_16BIT_WORD(buffer, BPB_RsvdSecCnt);
    Number_of_FATS = buffer[BPB_NumFATs];
    impl->FAT32.fat_sectors = GET_32BIT_WORD(buffer, BPB_FAT32_FATSz32);
    impl->FAT32.RootDir_First_Cluster = GET_32BIT_WORD(buffer, BPB_FAT32_RootClus);
    impl->FAT32.fs_info_sector = GET_16BIT_WORD(buffer, BPB_FAT32_FSInfo);

    // First FAT LBA address
    impl->FAT32.fat_begin_lba = LbaBegin + Reserved_Sectors;

    // The address of the first data cluster on this volume
    impl->FAT32.cluster_begin_lba = impl->FAT32.fat_begin_lba + (Number_of_FATS * impl->FAT32.fat_sectors);

    if (GET_16BIT_WORD(buffer, 0x1FE)!=0xAA55) // This signature should be AA55
        return FALSE;

    // Calculate the root dir sectors
    RootDirSectors = ((GET_16BIT_WORD(buffer, BPB_RootEntCnt) * 32) + (GET_16BIT_WORD(buffer, BPB_BytsPerSec) - 1)) / GET_16BIT_WORD(buffer, BPB_BytsPerSec);

    if(GET_16BIT_WORD(buffer, BPB_FATSz16) != 0)
        FATSz = GET_16BIT_WORD(buffer, BPB_FATSz16);
    else
        FATSz = GET_32BIT_WORD(buffer, BPB_FAT32_FATSz32);

    if(GET_16BIT_WORD(buffer, BPB_TotSec16) != 0)
        TotSec = GET_16BIT_WORD(buffer, BPB_TotSec16);
    else
        TotSec = GET_32BIT_WORD(buffer, BPB_TotSec32);

    DataSec = TotSec - (GET_16BIT_WORD(buffer, BPB_RsvdSecCnt) + (buffer[BPB_NumFATs] * FATSz) + RootDirSectors);

    CountofClusters = DataSec / impl->FAT32.SectorsPerCluster;

    if(CountofClusters < 4085)
    {
        printf("FAT12\n");
        // Volume is FAT12
        return FALSE;
    }

    else if(CountofClusters < 65525)
    {
        printf("FAT16\n");
        // Volume is FAT16
        return FALSE;
    }

    return TRUE;
}
#endif // HAVE_UNIX
