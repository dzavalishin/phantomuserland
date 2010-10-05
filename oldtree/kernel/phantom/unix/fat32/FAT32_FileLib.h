#include "unix/fat32/define.h"
#include "FAT32_Opts.h"

#ifndef __FILELIB_H__
#define __FILELIB_H__

//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------
#ifndef SEEK_CUR
	#define SEEK_CUR    1
#endif
#ifndef SEEK_END
	#define SEEK_END    2
#endif
#ifndef SEEK_SET
	#define SEEK_SET    0
#endif

//-----------------------------------------------------------------------------
// Global Structures
//-----------------------------------------------------------------------------
typedef struct 
{
	UINT32	parentcluster;
	UINT32	startcluster;
	UINT32	bytenum;
	UINT32  currentBlock;
	UINT32  filelength;
	char	path[MAX_LONG_FILENAME];
	char	filename[MAX_LONG_FILENAME];
	BYTE	filebuf[512];
	BYTE	shortfilename[11];
	BOOL	inUse;
	BOOL	inRoot;

	BOOL	Read;
	BOOL	Write;
	BOOL	Append;
	BOOL	Binary;
	BOOL	Erase;
} FL_FILE;

//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------

// External
void				fl_shutdown();
FL_FILE*			fl_fopen(char *path, char *modifiers);
void				fl_fclose(FL_FILE *file);
int					fl_fgetc(FL_FILE *file);
int					fl_fputc(int c, FL_FILE *file);
int					fl_fputs(const char * str, FL_FILE *file);
int					fl_fwrite(const void * data, int size, int count, FL_FILE *file );
int					fl_fread(FL_FILE *file, BYTE * buffer, UINT32 count);
int					fl_fseek( FL_FILE *file , UINT32 offset , int origin );
int					fl_fgetpos(FL_FILE *file , UINT32 * position);
int					fl_remove( const char * filename );			

#endif
