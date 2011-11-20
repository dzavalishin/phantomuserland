#ifndef __FILESTRING_H__
#define __FILESTRING_H__

#include "unix/fat32/define.h"

//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
int FileString_PathTotalLevels(char *path);
int FileString_GetSubString(char *Path, int levelreq, char *output);
int FileString_SplitPath(char *FullPath, char *Path, char *FileName);
int FileString_StrCmpNoCase(char *s1, char *s2, int n);
int FileString_GetExtension(char *str);
int FileString_TrimLength(char *str, int strLen);
int FileString_Compare(char* strA, char* strB);

#ifndef NULL
	#define NULL 0
#endif

#endif