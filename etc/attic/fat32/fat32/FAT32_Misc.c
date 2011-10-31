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
#include "FAT32_Misc.h"

#include <phantom_libc.h>

//-----------------------------------------------------------------------------
// FATMisc_ClearLFN: Clear long file name cache
//-----------------------------------------------------------------------------
void FATMisc_ClearLFN(f32_t *impl, u_int8_t  wipeTable)
{
    int i;
    impl->FAT32_LFN.no_of_strings = 0;

    // Zero out buffer also
    if (wipeTable)
        for (i=0;i<MAX_LONGFILENAME_ENTRIES;i++)
            memset(impl->FAT32_LFN.String[i], 0x00, 13);
}
//-----------------------------------------------------------------------------
// FATMisc_cacheLFN - Function extracts long file name text from sector
//                       at a specific offset
//-----------------------------------------------------------------------------
void FATMisc_CacheLFN(f32_t *impl, BYTE *entryBuffer)
{
    BYTE LFNIndex, i;
    LFNIndex = entryBuffer[0] & 0x0F;

    if (impl->FAT32_LFN.no_of_strings==0)
        impl->FAT32_LFN.no_of_strings = LFNIndex;

    impl->FAT32_LFN.String[LFNIndex-1][0] = entryBuffer[1];
    impl->FAT32_LFN.String[LFNIndex-1][1] = entryBuffer[3];
    impl->FAT32_LFN.String[LFNIndex-1][2] = entryBuffer[5];
    impl->FAT32_LFN.String[LFNIndex-1][3] = entryBuffer[7];
    impl->FAT32_LFN.String[LFNIndex-1][4] = entryBuffer[9];
    impl->FAT32_LFN.String[LFNIndex-1][5] = entryBuffer[0x0E];
    impl->FAT32_LFN.String[LFNIndex-1][6] = entryBuffer[0x10];
    impl->FAT32_LFN.String[LFNIndex-1][7] = entryBuffer[0x12];
    impl->FAT32_LFN.String[LFNIndex-1][8] = entryBuffer[0x14];
    impl->FAT32_LFN.String[LFNIndex-1][9] = entryBuffer[0x16];
    impl->FAT32_LFN.String[LFNIndex-1][10] = entryBuffer[0x18];
    impl->FAT32_LFN.String[LFNIndex-1][11] = entryBuffer[0x1C];
    impl->FAT32_LFN.String[LFNIndex-1][12] = entryBuffer[0x1E];

    for (i=0; i<13; i++)
        if (impl->FAT32_LFN.String[LFNIndex-1][i]==0xFF)
            impl->FAT32_LFN.String[LFNIndex-1][i] = 0x20; // Replace with spaces
}
//-----------------------------------------------------------------------------
// FATMisc_GetLFNCache: Get a copy of the long filename to into a string buffer
//-----------------------------------------------------------------------------
void FATMisc_GetLFNCache(f32_t *impl, BYTE *strOut)
{
    int i,index;
    int lfncount = 0;

    // Copy LFN from LFN Cache into a string
    for (index=0;index<impl->FAT32_LFN.no_of_strings;index++)
        for (i=0; i<13; i++)
            strOut[lfncount++] = impl->FAT32_LFN.String[index][i];

    // Null terminate string
    strOut[lfncount]='\0';
}
//-----------------------------------------------------------------------------
// FATMisc_If_LFN_TextOnly: If LFN text entry found
//-----------------------------------------------------------------------------
int FATMisc_If_LFN_TextOnly(f32_t *impl, FAT32_ShortEntry *entry)
{
    (void) impl;

    if ((entry->Attr&0x0F)==FILE_ATTR_LFN_TEXT)
        return 1;
    else
        return 0;
}
//-----------------------------------------------------------------------------
// FATMisc_If_LFN_Invalid: If SFN found not relating to LFN
//-----------------------------------------------------------------------------
int FATMisc_If_LFN_Invalid(f32_t *impl, FAT32_ShortEntry *entry)
{
    (void) impl;

    if ((entry->Name[0]==FILE_HEADER_BLANK)||(entry->Name[0]==FILE_HEADER_DELETED)||(entry->Attr==FILE_ATTR_VOLUME_ID)||(entry->Attr&FILE_ATTR_SYSHID))
        return 1;
    else
        return 0;
}
//-----------------------------------------------------------------------------
// FATMisc_If_LFN_Exists: If LFN exists and correlation SFN found
//-----------------------------------------------------------------------------
int FATMisc_If_LFN_Exists(f32_t *impl, FAT32_ShortEntry *entry)
{
    if ((entry->Attr!=FILE_ATTR_LFN_TEXT) && (entry->Name[0]!=FILE_HEADER_BLANK) && (entry->Name[0]!=FILE_HEADER_DELETED) && (entry->Attr!=FILE_ATTR_VOLUME_ID) && (!(entry->Attr&FILE_ATTR_SYSHID)) && (impl->FAT32_LFN.no_of_strings))
        return 1;
    else
        return 0;
}
//-----------------------------------------------------------------------------
// FATMisc_If_noLFN_SFN_Only: If SFN only exists
//-----------------------------------------------------------------------------
int FATMisc_If_noLFN_SFN_Only(f32_t *impl, FAT32_ShortEntry *entry)
{
    (void) impl;

    if ((entry->Attr!=FILE_ATTR_LFN_TEXT) && (entry->Name[0]!=FILE_HEADER_BLANK) && (entry->Name[0]!=FILE_HEADER_DELETED) && (entry->Attr!=FILE_ATTR_VOLUME_ID) && (!(entry->Attr&FILE_ATTR_SYSHID)))
        return 1;
    else
        return 0;
}
//-----------------------------------------------------------------------------
// FATMisc_If_dir_entry: Returns 1 if a directory
//-----------------------------------------------------------------------------
int FATMisc_If_dir_entry(f32_t *impl, FAT32_ShortEntry *entry)
{
    (void) impl;

    if (entry->Attr&FILE_TYPE_DIR)
        return 1;
    else
        return 0;
}
//-----------------------------------------------------------------------------
// FATMisc_If_file_entry: Returns 1 is a file entry
//-----------------------------------------------------------------------------
int FATMisc_If_file_entry(f32_t *impl, FAT32_ShortEntry *entry)
{
    (void) impl;

    if (entry->Attr&FILE_TYPE_FILE)
        return 1;
    else
        return 0;
}
//-----------------------------------------------------------------------------
// FATMisc_LFN_to_entry_count:
//-----------------------------------------------------------------------------
int FATMisc_LFN_to_entry_count(char *filename)
{
    // 13 characters entries
    int length = (int)strlen(filename);
    int entriesRequired = length / 13;

    // Remainder
    if ((length % 13)!=0)
        entriesRequired++;

    return entriesRequired;
}
//-----------------------------------------------------------------------------
// FATMisc_LFN_to_lfn_entry:
//-----------------------------------------------------------------------------
void FATMisc_LFN_to_lfn_entry(f32_t *impl, char *filename, BYTE *buffer, int entry, BYTE sfnChk)
{
    (void) impl;

    int i;
    int nameIndexes[] = {1,3,5,7,9,0x0E,0x10,0x12,0x14,0x16,0x18,0x1C,0x1E};

    // 13 characters entries
    int length = (int)strlen(filename);
    int entriesRequired = FATMisc_LFN_to_entry_count(filename);

    // Filename offset
    int start = entry * 13;

    // Initialise to zeros
    memset(buffer, 0x00, 32);

    // LFN entry number
    buffer[0] = (BYTE)(((entriesRequired-1)==entry)?(0x40|(entry+1)):(entry+1));

    // LFN flag
    buffer[11] = 0x0F;

    // Checksum of short filename
    buffer[13] = sfnChk;

    // Copy to buffer
    for (i=0;i<13;i++)
    {
        if ( (start+i) < length )
            buffer[nameIndexes[i]] = filename[start+i];
        else if ( (start+i) == length )
            buffer[nameIndexes[i]] = 0x00;
        else
        {
            buffer[nameIndexes[i]] = 0xFF;
            buffer[nameIndexes[i]+1] = 0xFF;
        }
    }


}
//-----------------------------------------------------------------------------
// FATMisc_Create_sfn_entry: Create the short filename directory entry
//-----------------------------------------------------------------------------
#ifdef INCLUDE_WRITE_SUPPORT
void FATMisc_Create_sfn_entry(f32_t *impl, char *shortfilename, UINT32 size, UINT32 startCluster, FAT32_ShortEntry *entry)
{
    (void) impl;

    int i;

    // Copy short filename
    for (i=0;i<11;i++)
        entry->Name[i] = shortfilename[i];

    // Unless we have a RTC we might as well set these to 1980
    entry->CrtTimeTenth = 0x00;
    entry->CrtTime[1] = entry->CrtTime[0] = 0x00;
    entry->CrtDate[1] = 0x00;
    entry->CrtDate[0] = 0x20;
    entry->LstAccDate[1] = 0x00;
    entry->LstAccDate[0] = 0x20;
    entry->WrtTime[1] = entry->WrtTime[0] = 0x00;
    entry->WrtDate[1] = 0x00;
    entry->WrtDate[0] = 0x20;

    entry->Attr = FILE_TYPE_FILE;
    entry->NTRes = 0x00;

    entry->FstClusHI = (UINT16)((startCluster>>16) & 0xFFFF);
    entry->FstClusLO = (UINT16)((startCluster>>0) & 0xFFFF);
    entry->FileSize = size;
}
#endif
//-----------------------------------------------------------------------------
// FATMisc_CreateSFN: Create a padded SFN
//-----------------------------------------------------------------------------
#ifdef INCLUDE_WRITE_SUPPORT
BOOL FATMisc_CreateSFN(f32_t *impl, char *sfn_output, char *filename)
{
    (void) impl;

    int i;
    int dotPos = -1;
    char ext[3];
    int pos;
    int len = (int)strlen(filename);

    // Invalid to start with .
    if (filename[0]=='.')
        return FALSE;

    memset(sfn_output, ' ', 11);
    memset(ext, ' ', 3);

    // Find dot seperator
    for (i = 0; i< len; i++)
    {
        if (filename[i]=='.')
            dotPos = i;
    }

    // Extract extensions
    if (dotPos!=-1)
    {
        // Copy first three chars of extension
        for (i = (dotPos+1); i < (dotPos+1+3); i++)
            if (i<len)
                ext[i-(dotPos+1)] = filename[i];

        // Shorten the length to the dot position
        len = dotPos;
    }

    // Add filename part
    pos = 0;
    for (i=0;i<len;i++)
    {
        if ( (filename[i]!=' ') && (filename[i]!='.') )
            sfn_output[pos++] = (char)toupper(filename[i]);

        // Fill upto 8 characters
        if (pos==8)
            break;
    }

    // Add extension part
    for (i=8;i<11;i++)
        sfn_output[i] = (char)toupper(ext[i-8]);

    return TRUE;
}
#endif
//-----------------------------------------------------------------------------
// FATMisc_GenerateTail:
// sfn_input = Input short filename, spaced format & in upper case
// sfn_output = Output short filename with tail
//-----------------------------------------------------------------------------

BOOL FATMisc_GenerateTail( f32_t *impl, char *sfn_output, char *sfn_input, UINT32 tailNum)
{
    (void) impl;

    int tail_chars;
    char tail_str[8];

    if (tailNum > 99999)
        return FALSE;

    // Convert to number
    memset(tail_str, 0x00, sizeof(tail_str));
    tail_str[0] = '~';
    //itoa(tailNum, tail_str+1, 10);
    snprintf( tail_str+1, 7, "%d", tailNum);

    // Copy in base filename
    memcpy(sfn_output, sfn_input, 11);

    // Overwrite with tail
    tail_chars = (int)strlen(tail_str);
    memcpy(sfn_output+(8-tail_chars), tail_str, tail_chars);

    return TRUE;
}

#endif // #if HAVE_UNIX

