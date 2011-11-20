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
#include "FAT32_Access.h"
#include "FAT32_Write.h"
#include "FAT32_FileString.h"
#include "FAT32_Misc.h"

//-----------------------------------------------------------------------------
// FAT32_InitFAT: Load FAT32 Parameters
//-----------------------------------------------------------------------------
BOOL FAT32_InitFAT(f32_t *impl, int rawDisk )
{
    impl->FATFS_Internal.SectorCurrentlyLoaded = 0xFFFFFFFF;
    impl->FATFS_Internal.NextFreeCluster = 0xFFFFFFFF;
    return FAT32_Init(impl, rawDisk );
}
//-----------------------------------------------------------------------------
// FAT32_SectorReader: From the provided startcluster and sector offset
// Returns True if success, returns False if not (including if read out of range)
//-----------------------------------------------------------------------------
BOOL FAT32_SectorReader(f32_t *impl, UINT32 Startcluster, UINT32 offset)
{
    UINT32 SectortoRead = 0;
    UINT32 ClustertoRead = 0;
    UINT32 ClusterChain = 0;
    UINT32 i;
    UINT32 lba;

    // Set start of cluster chain to initial value
    ClusterChain = Startcluster;

    // Find parameters
    ClustertoRead = offset / impl->FAT32.SectorsPerCluster;
    SectortoRead = offset - (ClustertoRead*impl->FAT32.SectorsPerCluster);

    // Follow chain to find cluster to read
    for (i=0; i<ClustertoRead; i++)
        ClusterChain = FAT32_FindNextCluster(impl,ClusterChain);

    // If end of cluster chain then return false
    if (ClusterChain==0xFFFFFFFF)
        return FALSE;

    // Calculate sector address
    lba = FAT32_LBAofCluster(impl, ClusterChain)+SectortoRead;

    // Else read sector if not already loaded
    if (lba!=impl->FATFS_Internal.SectorCurrentlyLoaded)
    {
        impl->FATFS_Internal.SectorCurrentlyLoaded = lba;
        return impl->FAT_ReadSector(impl,impl->FATFS_Internal.SectorCurrentlyLoaded, impl->FATFS_Internal.currentsector);
    }
    else
        return TRUE;
}

//-----------------------------------------------------------------------------
// FAT32_SectorWriter: Write to the provided startcluster and sector offset
// Returns True if success, returns False if not
//-----------------------------------------------------------------------------

BOOL FAT32_SectorWriter(f32_t *impl, UINT32 Startcluster, UINT32 offset)
{
    UINT32 SectortoWrite = 0;
    UINT32 ClustertoWrite = 0;
    UINT32 ClusterChain = 0;
    UINT32 LastClusterChain = 0xFFFFFFFF;
    UINT32 i;

    // Set start of cluster chain to initial value
    ClusterChain = Startcluster;

    // Find parameters
    ClustertoWrite = offset / impl->FAT32.SectorsPerCluster;
    SectortoWrite = offset - (ClustertoWrite*impl->FAT32.SectorsPerCluster);

    // Follow chain to find cluster to read
    for (i=0; i<ClustertoWrite; i++)
    {
        // Find next link in the chain
        LastClusterChain = ClusterChain;
        ClusterChain = FAT32_FindNextCluster(impl,ClusterChain);

        // Dont keep following a dead end
        if (ClusterChain==0xFFFFFFFF)
            break;
    }

    // If end of cluster chain
    if (ClusterChain==0xFFFFFFFF)
    {
        // Add another cluster to the last good cluster chain
        if (!FAT32_AddFreeSpaceToChain(impl,&LastClusterChain))
            return FALSE;

        ClusterChain = LastClusterChain;
    }

    // Calculate write address
    impl->FATFS_Internal.SectorCurrentlyLoaded = FAT32_LBAofCluster(impl, ClusterChain)+SectortoWrite;

    // Write to disk
    return impl->FAT_WriteSector(impl, impl->FATFS_Internal.SectorCurrentlyLoaded, impl->FATFS_Internal.currentsector);
}

//-----------------------------------------------------------------------------
// FAT32_ShowFATDetails: Show the details about the filesystem
//-----------------------------------------------------------------------------
void FAT32_ShowFATDetails( f32_t *impl )
{
    printf("\r\nCurrent Disc FAT details\r\n------------------------\r\nRoot Dir First Cluster = ");
    printf("0x%x",impl->FAT32.RootDir_First_Cluster);
    printf("\r\nFAT Begin LBA = ");
    printf("0x%x",impl->FAT32.fat_begin_lba);
    printf("\r\nCluster Begin LBA = ");
    printf("0x%x",impl->FAT32.cluster_begin_lba);
    printf("\r\nSectors Per Cluster = ");
    printf("%d",impl->FAT32.SectorsPerCluster);
    printf("\r\n\r\nFormula for conversion from Cluster num to LBA is;");
    printf("\r\nLBA = (cluster_begin_lba + ((Cluster_Number-2)*SectorsPerCluster)))\r\n");
}
//-----------------------------------------------------------------------------
// FAT32_GetRootCluster: Get the root dir cluster
//-----------------------------------------------------------------------------
UINT32 FAT32_GetRootCluster(f32_t *impl)
{
    return impl->FAT32.RootDir_First_Cluster;
}
//-------------------------------------------------------------
// FAT32_GetFileEntry: Find the file entry for a filename
//-------------------------------------------------------------
UINT32 FAT32_GetFileEntry(f32_t *impl, UINT32 Cluster, char *nametofind, FAT32_ShortEntry *sfEntry)
{
    BYTE item=0;
    UINT16 recordoffset = 0;
    BYTE i=0;
    int x=0;
    char LongFilename[MAX_LONG_FILENAME];
    char ShortFilename[13];
    FAT32_ShortEntry *directoryEntry;

    FATMisc_ClearLFN(impl, TRUE);

    // Main cluster following loop
    while (TRUE)
    {
        // Read sector
        if (FAT32_SectorReader(impl, Cluster, x++)) // If sector read was successfull
        {
            // Analyse Sector
            for (item=0; item<=15;item++)
            {
                // Create the multiplier for sector access
                recordoffset = (32*item);

                // Overlay directory entry over buffer
                directoryEntry = (FAT32_ShortEntry*)(impl->FATFS_Internal.currentsector+recordoffset);

                // Long File Name Text Found
                if (FATMisc_If_LFN_TextOnly(impl, directoryEntry) )
                    FATMisc_CacheLFN(impl, impl->FATFS_Internal.currentsector+recordoffset);

                // If Invalid record found delete any long file name information collated
                else if (FATMisc_If_LFN_Invalid(impl, directoryEntry) )
                    FATMisc_ClearLFN(impl, FALSE);

                // Normal SFN Entry and Long text exists
                else if (FATMisc_If_LFN_Exists(impl, directoryEntry) )
                {
                    FATMisc_GetLFNCache(impl, (BYTE*)LongFilename);

                    // Compare names to see if they match
                    if (FileString_Compare(LongFilename, nametofind))
                    {
                        memcpy(sfEntry,directoryEntry,sizeof(FAT32_ShortEntry));
                        return TRUE;
                    }

                    FATMisc_ClearLFN(impl, FALSE);
                }

                // Normal Entry, only 8.3 Text
                else if (FATMisc_If_noLFN_SFN_Only(impl, directoryEntry) )
                {
                    memset(ShortFilename, 0, sizeof(ShortFilename));

                    // Copy name to string
                    for (i=0; i<8; i++)
                        ShortFilename[i] = directoryEntry->Name[i];

                    // If not . or .. entry
                    if (ShortFilename[0]!='.')
                        ShortFilename[8] = '.';
                    else
                        ShortFilename[8] = ' ';

                    // Extension
                    for (i=8; i<11; i++)
                        ShortFilename[i+1] = directoryEntry->Name[i];

                    // Compare names to see if they match
                    if (FileString_Compare(ShortFilename, nametofind))
                    {
                        memcpy(sfEntry,directoryEntry,sizeof(FAT32_ShortEntry));
                        return TRUE;
                    }

                    FATMisc_ClearLFN(impl, FALSE);
                }
            } // End of if
        }
        else
            break;
    } // End of while loop

    return FALSE;
}
//-------------------------------------------------------------
// FAT32_SFNexists: Check if a short filename exists.
// NOTE: shortname is XXXXXXXXYYY not XXXXXXXX.YYY
//-------------------------------------------------------------
#ifdef INCLUDE_WRITE_SUPPORT
BOOL FAT32_SFNexists(f32_t *impl, UINT32 Cluster, char *shortname)
{
    BYTE item=0;
    UINT16 recordoffset = 0;
    int x=0;
    FAT32_ShortEntry *directoryEntry;

    // Main cluster following loop
    while (TRUE)
    {
        // Read sector
        if (FAT32_SectorReader(impl, Cluster, x++)) // If sector read was successfull
        {
            // Analyse Sector
            for (item=0; item<=15;item++)
            {
                // Create the multiplier for sector access
                recordoffset = (32*item);

                // Overlay directory entry over buffer
                directoryEntry = (FAT32_ShortEntry*)(impl->FATFS_Internal.currentsector+recordoffset);

                // Long File Name Text Found
                if (FATMisc_If_LFN_TextOnly(impl, directoryEntry) )
                    ;

                // If Invalid record found delete any long file name information collated
                else if (FATMisc_If_LFN_Invalid(impl, directoryEntry) )
                    ;

                // Normal Entry, only 8.3 Text
                else if (FATMisc_If_noLFN_SFN_Only(impl, directoryEntry) )
                {
                    if (strncmp((const char*)directoryEntry->Name, shortname, 11)==0)
                        return TRUE;
                }
            } // End of if
        }
        else
            break;
    } // End of while loop

    return FALSE;
}
#endif
//-------------------------------------------------------------
// FAT32_UpdateFileLength: Find a SFN entry and update it
// NOTE: shortname is XXXXXXXXYYY not XXXXXXXX.YYY
//-------------------------------------------------------------
BOOL FAT32_UpdateFileLength(f32_t *impl, UINT32 Cluster, char *shortname, UINT32 fileLength)
{
    BYTE item=0;
    UINT16 recordoffset = 0;
    int x=0;
    FAT32_ShortEntry *directoryEntry;

    // Main cluster following loop
    while (TRUE)
    {
        // Read sector
        if (FAT32_SectorReader(impl, Cluster, x++)) // If sector read was successfull
        {
            // Analyse Sector
            for (item=0; item<=15;item++)
            {
                // Create the multiplier for sector access
                recordoffset = (32*item);

                // Overlay directory entry over buffer
                directoryEntry = (FAT32_ShortEntry*)(impl->FATFS_Internal.currentsector+recordoffset);

                // Long File Name Text Found
                if (FATMisc_If_LFN_TextOnly(impl, directoryEntry) )
                    ;

                // If Invalid record found delete any long file name information collated
                else if (FATMisc_If_LFN_Invalid(impl, directoryEntry) )
                    ;

                // Normal Entry, only 8.3 Text
                else if (FATMisc_If_noLFN_SFN_Only(impl, directoryEntry) )
                {
                    if (strncmp((const char*)directoryEntry->Name, shortname, 11)==0)
                    {
                        directoryEntry->FileSize = fileLength;
                        // TODO: Update last write time

                        // Update sfn entry
                        memcpy((BYTE*)(impl->FATFS_Internal.currentsector+recordoffset), (BYTE*)directoryEntry, sizeof(FAT32_ShortEntry));

                        // Write sector back
                        return impl->FAT_WriteSector(impl, impl->FATFS_Internal.SectorCurrentlyLoaded, impl->FATFS_Internal.currentsector);
                    }
                }
            } // End of if
        }
        else
            break;
    } // End of while loop

    return FALSE;
}
//-------------------------------------------------------------
// FAT32_MarkFileDeleted: Find a SFN entry and mark if as deleted
// NOTE: shortname is XXXXXXXXYYY not XXXXXXXX.YYY
//-------------------------------------------------------------
BOOL FAT32_MarkFileDeleted(f32_t *impl, UINT32 Cluster, char *shortname)
{
    BYTE item=0;
    UINT16 recordoffset = 0;
    int x=0;
    FAT32_ShortEntry *directoryEntry;

    // Main cluster following loop
    while (TRUE)
    {
        // Read sector
        if (FAT32_SectorReader(impl, Cluster, x++)) // If sector read was successfull
        {
            // Analyse Sector
            for (item=0; item<=15;item++)
            {
                // Create the multiplier for sector access
                recordoffset = (32*item);

                // Overlay directory entry over buffer
                directoryEntry = (FAT32_ShortEntry*)(impl->FATFS_Internal.currentsector+recordoffset);

                // Long File Name Text Found
                if (FATMisc_If_LFN_TextOnly(impl, directoryEntry) )
                    ;

                // If Invalid record found delete any long file name information collated
                else if (FATMisc_If_LFN_Invalid(impl, directoryEntry) )
                    ;

                // Normal Entry, only 8.3 Text
                else if (FATMisc_If_noLFN_SFN_Only(impl, directoryEntry) )
                {
                    if (strncmp((const char *)directoryEntry->Name, shortname, 11)==0)
                    {
                        // Mark as deleted
                        directoryEntry->Name[0] = 0xE5;

                        // Update sfn entry
                        memcpy((BYTE*)(impl->FATFS_Internal.currentsector+recordoffset), (BYTE*)directoryEntry, sizeof(FAT32_ShortEntry));

                        // Write sector back
                        return impl->FAT_WriteSector(impl, impl->FATFS_Internal.SectorCurrentlyLoaded, impl->FATFS_Internal.currentsector);
                    }
                }
            } // End of if
        }
        else
            break;
    } // End of while loop

    return FALSE;
}
//-----------------------------------------------------------------------------
// ListDirectory: Using starting cluster number of a directory and the FAT,
//				  list all directories and files
//-----------------------------------------------------------------------------
void ListDirectory(f32_t *impl, UINT32 StartCluster)
{
    BYTE i,item;
    UINT16 recordoffset;
    BYTE LFNIndex=0;
    UINT32 x=0;
    FAT32_ShortEntry *directoryEntry;
    char LongFilename[MAX_LONG_FILENAME];
    char ShortFilename[13];

    impl->FAT32.filenumber=0;
    printf("\r\nNo.             Filename\r\n");

    FATMisc_ClearLFN(impl, TRUE);

    while (TRUE)
    {
        // If data read OK
        if (FAT32_SectorReader(impl, StartCluster, x++))
        {
            LFNIndex=0;

            // Maximum of 15 directory entries
            for (item=0; item<=15;item++)
            {
                // Increase directory offset
                recordoffset = (32*item);

                // Overlay directory entry over buffer
                directoryEntry = (FAT32_ShortEntry*)(impl->FATFS_Internal.currentsector+recordoffset);

                // Long File Name Text Found
                if ( FATMisc_If_LFN_TextOnly(impl, directoryEntry) )
                    FATMisc_CacheLFN(impl, impl->FATFS_Internal.currentsector+recordoffset);

                // If Invalid record found delete any long file name information collated
                else if ( FATMisc_If_LFN_Invalid(impl, directoryEntry) )
                    FATMisc_ClearLFN(impl, FALSE);

                // Normal SFN Entry and Long text exists
                else if (FATMisc_If_LFN_Exists(impl, directoryEntry) )
                {
                    impl->FAT32.filenumber++; //File / Dir Count

                    // Get text
                    FATMisc_GetLFNCache(impl, (BYTE*)LongFilename);

                    if (FATMisc_If_dir_entry(impl, directoryEntry)) printf("\r\nDirectory ");
                    if (FATMisc_If_file_entry(impl, directoryEntry)) printf("\r\nFile ");

                    // Print Filename
                    printf("%d - %s [%d bytes] (0x%08lx)",impl->FAT32.filenumber, LongFilename, directoryEntry->FileSize, (directoryEntry->FstClusHI<<16)|directoryEntry->FstClusLO);

                    FATMisc_ClearLFN(impl, FALSE);
                }

                // Normal Entry, only 8.3 Text
                else if ( FATMisc_If_noLFN_SFN_Only(impl, directoryEntry) )
                {
                    FATMisc_ClearLFN(impl, FALSE);
                    impl->FAT32.filenumber++; //File / Dir Count

                    if (FATMisc_If_dir_entry(impl, directoryEntry)) printf("\r\nDirectory ");
                    if (FATMisc_If_file_entry(impl, directoryEntry)) printf("\r\nFile ");

                    memset(ShortFilename, 0, sizeof(ShortFilename));

                    // Copy name to string
                    for (i=0; i<8; i++)
                        ShortFilename[i] = directoryEntry->Name[i];

                    // If not . or .. entry
                    if (ShortFilename[0]!='.')
                        ShortFilename[8] = '.';
                    else
                        ShortFilename[8] = ' ';

                    // Extension
                    for (i=8; i<11; i++)
                        ShortFilename[i+1] = directoryEntry->Name[i];

                    // Print Filename
                    printf("%d - %s",impl->FAT32.filenumber, ShortFilename);

                }
            }// end of for
        }
        else
            break;
    }
}

#endif // HAVE_UNIX

