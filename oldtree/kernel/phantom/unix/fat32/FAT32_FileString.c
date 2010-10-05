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
#include "FAT32_FileString.h"

//-----------------------------------------------------------------------------
// FileString_PathTotalLevels: Take a filename and path and count the sub levels
// of folders. E.g. C:\folder\file.zip = 1 level
// Returns: -1 = Error, 0 or more = Ok
//-----------------------------------------------------------------------------
int FileString_PathTotalLevels(char *path)
{
    int levels=0;

    int length = (int)strlen(path);

    // If too short
    if (length<3)
        return -1;

    // Check for C:\...
    if (path[1]!=':' || path[2]!='\\')
        return -1;

    // Count levels in path string
    while (*path)
    {
        // Fast forward through actual subdir text to next slash
        for(;*path;)
        {
            // If slash detected escape from for loop
            if (*path == '\\') { path++; break; }
            path++;
        }

        // Increase number of subdirs founds
        levels++;
    }

    // Subtract the drive letter level and file itself
    return levels-2;
}
//-----------------------------------------------------------------------------
// FileString_GetSubString: Get a substring from 'Path' which contains the folder
// (or file) at the specified level.
// E.g. C:\folder\file.zip : Level 0 = C:\folder, Level 1 = file.zip
// Returns: -1 = Error, 0 = Ok
//-----------------------------------------------------------------------------
int FileString_GetSubString(char *Path, int levelreq, char *output)
{
    int i;
    int pathlen=0;
    int levels=0;
    int copypnt=0;

    // Get string length of Path
    pathlen = (int)strlen (Path);

    // If too short
    if (pathlen<3)
        return -1;

    // Check for C:\...
    if (Path[1]!=':' || Path[2]!='\\')
        return -1;

    // Loop through the number of times as characters in 'Path'
    for (i = 0; i<pathlen; i++)
    {
        // If a '\' is found then increase level
        if (*Path=='\\') levels++;

        // If correct level and the character is not a '\' then copy text to 'output'
        if ( (levels==(levelreq+1)) && (*Path!='\\') )
            output[copypnt++] = *Path;

        // Increment through path string
        Path++;
    }

    // Null Terminate
    output[copypnt] = '\0';

    // If a string was copied return 0 else return 1
    if (output[0]!='\0')
        return 0;	// OK
    else
        return -1;	// Error
}
//-----------------------------------------------------------------------------
// FileString_SplitPath: Full path contains the passed in string.
// Returned is the Path string and file Name string
// E.g. C:\folder\file.zip -> Path = C:\folder  FileName = file.zip
// E.g. C:\file.zip -> Path = [blank]  FileName = file.zip
//-----------------------------------------------------------------------------
int FileString_SplitPath(char *FullPath, char *Path, char *FileName)
{
    int strindex;

    // Count the levels to the filepath
    int levels = FileString_PathTotalLevels(FullPath);
    if (levels==-1)
        return -1;

    // Get filename part of string
    FileString_GetSubString(FullPath, levels, FileName);

    // If root file
    if (levels==0)
    {
        Path[0]='\0';
        return 0;
    }
    else
    {
        strindex = (int)strlen(FullPath) - (int)strlen(FileName);
        memcpy(Path, FullPath, strindex);
        Path[strindex-1] = '\0';
        return 0;
    }
}
//-----------------------------------------------------------------------------
// FileString_StrCmpNoCase: Compare two strings case with case sensitivity
//-----------------------------------------------------------------------------
int FileString_StrCmpNoCase(char *s1, char *s2, int n)
{
    int diff;
    char a,b;

    while (n--)
    {
        a = *s1;
        b = *s2;

        // Make lower case if uppercase
        if ((a>='A') && (a<='Z'))
            a+= 32;
        if ((b>='A') && (b<='Z'))
            b+= 32;

        diff = a - b;

        // If different
        if (diff)
            return diff;

        // If run out of strings
        if ( (*s1 == 0) || (*s2 == 0) )
            break;

        s1++;
        s2++;
    }
    return 0;
}
//-----------------------------------------------------------------------------
// FileString_GetExtension: Get index to extension within filename
// Returns -1 if not found or index otherwise
//-----------------------------------------------------------------------------
int FileString_GetExtension(char *str)
{
    int dotPos = -1;
    char *strSrc = str;

    // Find last '.' in string (if at all)
    while (*strSrc)
    {
        if (*strSrc=='.')
            dotPos = (int)(strSrc-str);

        strSrc++;
    }

    return dotPos;
}
//-----------------------------------------------------------------------------
// FileString_TrimLength: Get length of string excluding trailing spaces
// Returns -1 if not found or index otherwise
//-----------------------------------------------------------------------------
int FileString_TrimLength(char *str, int strLen)
{
    int length = strLen;
    char *strSrc = str+strLen-1;

    // Find last non white space
    while (strLen!=0)
    {
        if (*strSrc==' ')
            length = (int)(strSrc - str);
        else
            break;

        strSrc--;
        strLen--;
    }

    return length;
}
//-----------------------------------------------------------------------------
// FileString_Compare: Compare two filenames (without copying or changing origonals)
// Returns 1 if match, 0 if not
//-----------------------------------------------------------------------------
int FileString_Compare(char* strA, char* strB)
{
    char *ext1 = NULL;
    char *ext2 = NULL;
    int ext1Pos, ext2Pos;
    int file1Len, file2Len;

    // Get both files extension
    ext1Pos = FileString_GetExtension(strA);
    ext2Pos = FileString_GetExtension(strB);

    // NOTE: Extension position can be different for matching
    // filename if trailing space are present before it!
    // Check that if one has an extension, so does the other
    if ((ext1Pos==-1) && (ext2Pos!=-1))
        return 0;
    if ((ext2Pos==-1) && (ext1Pos!=-1))
        return 0;

    // If they both have extensions, compare them
    if (ext1Pos!=-1)
    {
        // Set pointer to start of extension
        ext1 = strA+ext1Pos+1;
        ext2 = strB+ext2Pos+1;

        // If they dont match
        if (FileString_StrCmpNoCase(ext1, ext2, (int)strlen(ext1))!=0)
            return 0;

        // Filelength is upto extensions
        file1Len = ext1Pos;
        file2Len = ext2Pos;
    }
    // No extensions
    else
    {
        // Filelength is actual filelength
        file1Len = (int)strlen(strA);
        file2Len = (int)strlen(strB);
    }

    // Find length without trailing spaces (before ext)
    file1Len = FileString_TrimLength(strA, file1Len);
    file2Len = FileString_TrimLength(strB, file2Len);

    // Check the file lengths match
    if (file1Len!=file2Len)
        return 0;

    // Compare main part of filenames
    if (FileString_StrCmpNoCase(strA, strB, file1Len)!=0)
        return 0;
    else
        return 1;
}

#endif // #if HAVE_UNIX
