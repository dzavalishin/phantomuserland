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

#if HAVE_UNIX


#include "FAT32_Definitions.h"
#include "FAT32_Base.h"
#include "FAT32_Table.h"
#include "FAT32_Access.h"
#include "FAT32_Write.h"
#include "FAT32_Misc.h"
#include "FAT32_FileString.h"
#include "FAT32_FileLib.h"

//-----------------------------------------------------------------------------
// Locals
//-----------------------------------------------------------------------------
static FL_FILE		Files[MAX_OPEN_FILES];
static int			Filelib_Init = FALSE;

// Macro for checking if file lib is initialised
#define CHECK_FL_INIT()		{ if (Filelib_Init==FALSE) _fl_init(); }

//-----------------------------------------------------------------------------
// Local Functions
//-----------------------------------------------------------------------------
static BOOL				_open_directory(char *path, UINT32 *pathCluster);
static FL_FILE*			_find_spare_file();
static void				_fl_init();
static FL_FILE*			_read_file(char *path);
static BOOL				_write_block(FL_FILE *file, BYTE *data, UINT32 length);
static FL_FILE*			_create_file(char *filename, UINT32 size);

//-----------------------------------------------------------------------------
// _fl_init: Initialise File Library
//-----------------------------------------------------------------------------
static void _fl_init()
{
	int i;
	for (i=0;i<MAX_OPEN_FILES;i++)
		Files[i].inUse = FALSE;

	Filelib_Init = TRUE;
}
//-----------------------------------------------------------------------------
// _find_spare_file: Find a slot in the open files buffer for a new file
//-----------------------------------------------------------------------------
static FL_FILE* _find_spare_file()
{
	int i;
	int freeFile = -1;
	for (i=0;i<MAX_OPEN_FILES;i++)
		if (Files[i].inUse == FALSE)
		{
			freeFile = i;
			break;
		}

	if (freeFile!=-1)
		return &Files[freeFile];
	else
		return NULL;
}
//-----------------------------------------------------------------------------
// _check_file_open: Returns true if the file is already open
//-----------------------------------------------------------------------------
static BOOL _check_file_open(FL_FILE* file)
{
	int i;

	if (file==NULL)
		return FALSE;

	// Compare open files
	for (i=0;i<MAX_OPEN_FILES;i++)
		if ( (Files[i].inUse) && (&Files[i]!=file) )
		{
			// Compare path and name
			if ( (FileString_Compare(Files[i].path,file->path)) && (FileString_Compare(Files[i].filename,file->filename)) )
				return TRUE;
		}

	return FALSE;
}
//-----------------------------------------------------------------------------
// _open_directory: Cycle through path string to find the start cluster
// address of the highest subdir.
//-----------------------------------------------------------------------------
static BOOL _open_directory(char *path, UINT32 *pathCluster)
{
	int levels;
	int sublevel;
	char currentfolder[MAX_LONG_FILENAME];
	FAT32_ShortEntry sfEntry;
	UINT32 startcluster;

	// Set starting cluster to root cluster
	startcluster = FAT32_GetRootCluster();

	// Find number of levels
	levels = FileString_PathTotalLevels(path);

	// Cycle through each level and get the start sector
	for (sublevel=0;sublevel<(levels+1);sublevel++) 
	{
		FileString_GetSubString(path, sublevel, currentfolder);

		// Find clusteraddress for folder (currentfolder) 
		if (FAT32_GetFileEntry(startcluster, currentfolder,&sfEntry))
			startcluster = (((UINT32)sfEntry.FstClusHI)<<16) + sfEntry.FstClusLO;
		else
			return FALSE;
	}

	*pathCluster = startcluster;
	return TRUE;
}

//-----------------------------------------------------------------------------
//								External API
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// fl_shutdown: Call before shutting down system
//-----------------------------------------------------------------------------
void fl_shutdown()
{
	// If first call to library, initialise
	CHECK_FL_INIT();

	FAT32_PurgeFATBuffer();
}
//-----------------------------------------------------------------------------
// fopen: Open or Create a file for reading or writing
//-----------------------------------------------------------------------------
FL_FILE* fl_fopen(char *path, char *mode)
{
	int modlen, i;
	FL_FILE* file; 

	BOOL read = FALSE;
	BOOL write = FALSE;
	BOOL append = FALSE;
	BOOL binary = FALSE;
	BOOL create = FALSE;
	BOOL erase = FALSE;

	// If first call to library, initialise
	CHECK_FL_INIT();

	if ((path==NULL) || (mode==NULL))
		return NULL;

	modlen = (int)strlen(mode);

	// Supported Modes:
	// "r" Open a file for reading. The file must exist. 
	// "w" Create an empty file for writing. If a file with the same name already exists its content is erased and the file is treated as a new empty file.  
	// "a" Append to a file. Writing operations append data at the end of the file. The file is created if it does not exist. 
	// "r+" Open a file for update both reading and writing. The file must exist. 
	// "w+" Create an empty file for both reading and writing. If a file with the same name already exists its content is erased and the file is treated as a new empty file. 
	// "a+" Open a file for reading and appending. All writing operations are performed at the end of the file, protecting the previous content to be overwritten. You can reposition (fseek, rewind) the internal pointer to anywhere in the file for reading, but writing operations will move it back to the end of file. The file is created if it does not exist. 

	for (i=0;i<modlen;i++)
	{
		switch (tolower(mode[i]))
		{
		case 'r':
			read = TRUE;
			break;
		case 'w':
			write = TRUE;
			erase = TRUE;
			create = TRUE;
			break;
		case 'a':
			write = TRUE;
			append = TRUE;
			create = TRUE;
			break;
		case '+':
			if (read)
				write = TRUE;
			else if (write)
			{
				read = TRUE;
				erase = TRUE;
				create = TRUE;
			}
			else if (append)
			{
				read = TRUE;
				write = TRUE;
				append = TRUE;
				create = TRUE;
			}
			break;
		case 'b':
			binary = TRUE;
			break;
		}
	}
	
	file = NULL;

	// Read
	if (read)
		file = _read_file(path);

	// Create New
#ifdef INCLUDE_WRITE_SUPPORT
	if ( (file==NULL) && (create) )
		file = _create_file(path, 0);
#else
	create = FALSE;
	write = FALSE;
	append = FALSE;
#endif

	// Write Existing
	if ( !create && !read && (write || append) )
		file = _read_file(path);

	if (file!=NULL)
	{
		file->Read = read;
		file->Write = write;
		file->Append = append;
		file->Binary = binary;
		file->Erase = erase;
	}

	return file;	
}
//-----------------------------------------------------------------------------
// _read_file: Open a file for reading
//-----------------------------------------------------------------------------
static FL_FILE* _read_file(char *path)
{
	FL_FILE* file; 
	FAT32_ShortEntry sfEntry;

	// If first call to library, initialise
	CHECK_FL_INIT();

	file = _find_spare_file();
	if (file==NULL)
		return NULL;

	// Clear filename
	memset(file->path, '\n', sizeof(file->path));
	memset(file->filename, '\n', sizeof(file->filename));

	// Split full path into filename and directory path
	FileString_SplitPath(path, file->path, file->filename);

	// Check if file already open
	if (_check_file_open(file))
		return FALSE;

	// If file is in the root dir
	if (file->path[0]==0)
	{
		file->parentcluster = FAT32_GetRootCluster();
		file->inRoot = TRUE;
	}
	else
	{
		file->inRoot = FALSE;

		// Find parent directory start cluster
		if (!_open_directory(file->path, &file->parentcluster))
			return NULL;
	}

	// Using dir cluster address search for filename
	if (FAT32_GetFileEntry(file->parentcluster, file->filename,&sfEntry))
	{
		// Initialise file details
		memcpy(file->shortfilename, sfEntry.Name, 11);
		file->filelength = sfEntry.FileSize;
		file->bytenum = 0;
		file->startcluster = (((UINT32)sfEntry.FstClusHI)<<16) + sfEntry.FstClusLO;
		file->currentBlock = 0xFFFFFFFF;
		file->inUse = TRUE;

		FAT32_PurgeFATBuffer();

		return file;
	}

	return NULL;
}
//-----------------------------------------------------------------------------
// fl_fclose: Close an open file
//-----------------------------------------------------------------------------
void fl_fclose(FL_FILE *file)
{
	// If first call to library, initialise
	CHECK_FL_INIT();

	if (file!=NULL)
	{
		file->bytenum = 0;
		file->filelength = 0;
		file->startcluster = 0;
		file->currentBlock = 0xFFFFFFFF;
		file->inUse = FALSE;

		FAT32_PurgeFATBuffer();
	}
}
//-----------------------------------------------------------------------------
// fl_fgetc: Get a character in the stream
//-----------------------------------------------------------------------------
int fl_fgetc(FL_FILE *file)
{
	UINT32 sector;
	UINT32 offset;	
	BYTE returnchar=0;

	// If first call to library, initialise
	CHECK_FL_INIT();

	if (file==NULL)
		return -1;

	// Check if file open
	if (file->inUse==FALSE)
		return -1;

	// No read permissions
	if (file->Read==FALSE)
		return -1;

	// Check if read past end of file
	if (file->bytenum>=file->filelength)
		return -1;

	// Calculations for file position
	sector = file->bytenum / 512;
	offset = file->bytenum - (sector*512);

	// If file block not already loaded
	if (file->currentBlock!=sector)
	{
		// Read the appropriate sector
		if (!FAT32_SectorReader(file->startcluster, sector)) 
			return -1;

		// Copy to file's buffer
        memcpy(file->filebuf, impl->FATFS_Internal.currentsector, 512);
		file->currentBlock=sector;
	}

	// Get the data block
	returnchar = file->filebuf[offset];

	// Increase next read position
	file->bytenum++;

	// Return character read
	return returnchar;
}
//-----------------------------------------------------------------------------
// fl_fread: Read a block of data from the file
//-----------------------------------------------------------------------------
int fl_fread (FL_FILE *file, BYTE * buffer, UINT32 count)
{
	UINT32 sector;
	UINT32 offset;
	UINT32 totalSectors;
	UINT32 bytesRead;
	UINT32 thisReadCount;
	UINT32 i;

	// If first call to library, initialise
	CHECK_FL_INIT();

	if (buffer==NULL || file==NULL)
		return -1;

	// Check if file open
	if (file->inUse==FALSE)
		return -1;

	// No read permissions
	if (file->Read==FALSE)
		return -1;

	// Nothing to be done
	if (count==0)
		return 0;

	// Check if read starts past end of file
	if (file->bytenum>=file->filelength)
		return -1;

	// Limit to file size
	if ( (file->bytenum + count) > file->filelength )
		count = file->filelength - file->bytenum;

	// Calculations for file position
	sector = file->bytenum / 512;
	offset = file->bytenum - (sector*512);

	// Calculate how many sectors this is
	totalSectors = (count+offset) / 512;

	// Take into account partial sector read
	if ((count+offset) % 512)
		totalSectors++;

	bytesRead = 0;
	for (i=0;i<totalSectors;i++)
	{
		// Read sector of file
		if ( FAT32_SectorReader(file->startcluster, (sector+i)) )
		{
			// Read length - full sector or remainder
			if ( (bytesRead+512) > count )
				thisReadCount = count - bytesRead;
			// First sector of multi-sector read
            else if(i == 0)
                thisReadCount = 512 - offset;
			else
				thisReadCount = 512;

			// Copy to file buffer (for continuation reads)
			memcpy(file->filebuf, impl->FATFS_Internal.currentsector, 512);
			file->currentBlock = (sector+i);

			// Copy to application buffer
			// Non aligned start
			if ( (i==0) && (offset!=0) )
				memcpy( (BYTE*)(buffer+bytesRead), (BYTE*)(file->filebuf+offset), thisReadCount);
			else
				memcpy( (BYTE*)(buffer+bytesRead), file->filebuf, thisReadCount);
		
			bytesRead+=thisReadCount;
			file->bytenum+=thisReadCount;

			if (thisReadCount>=count)
				return bytesRead;
		}
		// Read failed - out of range (probably)
		else
		{
			return (int)bytesRead;
		}
	}

	return bytesRead;
}
//-----------------------------------------------------------------------------
// fl_fseek: Seek to a specific place in the file
// TODO: This should support -ve numbers with SEEK END and SEEK CUR
//-----------------------------------------------------------------------------
int fl_fseek( FL_FILE *file , UINT32 offset , int origin )
{
	// If first call to library, initialise
	CHECK_FL_INIT();

	if (file==NULL)
		return -1;

	// Check if file open
	if (file->inUse==FALSE)
		return -1;

	if ( (origin == SEEK_END) && (offset!=0) )
		return -1;

	// Invalidate file buffer
	file->currentBlock = 0xFFFFFFFF;

	if (origin==SEEK_SET)
	{
		file->bytenum = offset;

		if (file->bytenum>file->filelength)
			file->bytenum = file->filelength;

		return 0;
	}
	else if (origin==SEEK_CUR)
	{
		file->bytenum+= offset;

		if (file->bytenum>file->filelength)
			file->bytenum = file->filelength;

		return 0;
	}
	else if (origin==SEEK_END)
	{
		file->bytenum = file->filelength;
		return 0;
	}
	else
		return -1;
}
//-----------------------------------------------------------------------------
// fl_fgetpos: Get the current file position
//-----------------------------------------------------------------------------
int fl_fgetpos(FL_FILE *file , UINT32 * position)
{
	if (file==NULL)
		return -1;

	// Check if file open
	if (file->inUse==FALSE)
		return -1;

	// Get position
	*position = file->bytenum;

	return 0;
}
//-----------------------------------------------------------------------------
// _create_file: Create a new file
//-----------------------------------------------------------------------------
#ifdef INCLUDE_WRITE_SUPPORT
static FL_FILE* _create_file(char *filename, UINT32 size)
{
	FL_FILE* file; 
	FAT32_ShortEntry sfEntry;
	char shortFilename[11];
	int tailNum;

	// If first call to library, initialise
	CHECK_FL_INIT();

	file = _find_spare_file();
	if (file==NULL)
		return NULL;

	// Clear filename
	memset(file->path, '\n', sizeof(file->path));
	memset(file->filename, '\n', sizeof(file->filename));

	// Split full path into filename and directory path
	FileString_SplitPath(filename, file->path, file->filename);

	// Check if file already open
	if (_check_file_open(file))
		return FALSE;

	// If file is in the root dir
	if (file->path[0]==0)
	{
		file->parentcluster = FAT32_GetRootCluster();
		file->inRoot = TRUE;
	}
	else
	{
		file->inRoot = FALSE;

		// Find parent directory start cluster
		if (!_open_directory(file->path, &file->parentcluster))
			return NULL;
	}

	// Check if same filename exists in directory
	if (FAT32_GetFileEntry(file->parentcluster, file->filename,&sfEntry)==TRUE)
		return NULL;

	// Create the file space for the file
	file->startcluster = 0;
	file->filelength = size;
	if (!FAT32_AllocateFreeSpace(TRUE, &file->startcluster, (file->filelength==0)?1:file->filelength))
		return NULL;

	// Generate a short filename & tail
	tailNum = 0;
	do 
	{
		// Create a standard short filename (without tail)
		FATMisc_CreateSFN(shortFilename, file->filename);

        // If second hit or more, generate a ~n tail		
		if (tailNum!=0)
			FATMisc_GenerateTail((char*)file->shortfilename, shortFilename, tailNum);
		// Try with no tail if first entry
		else
			memcpy(file->shortfilename, shortFilename, 11);

		// Check if entry exists already or not
		if (FAT32_SFNexists(file->parentcluster, (char*)file->shortfilename)==FALSE)
			break;

		tailNum++;
	}
	while (tailNum<9999);
	if (tailNum==9999)
		return NULL;

	// Add file to disk
	if (!FAT32_AddFileEntry(file->parentcluster, (char*)file->filename, (char*)file->shortfilename, file->startcluster, file->filelength))
		return NULL;

	// General
	file->bytenum = 0;
	file->currentBlock = 0xFFFFFFFF;
	file->inUse = TRUE;
	
	FAT32_PurgeFATBuffer();

	return file;
}
#endif
//-----------------------------------------------------------------------------
// fl_fputc: Write a character to the stream
//-----------------------------------------------------------------------------
#ifdef INCLUDE_WRITE_SUPPORT
int fl_fputc(int c, FL_FILE *file)
{
	BYTE Buffer[1];

	// If first call to library, initialise
	CHECK_FL_INIT();

	if (file==NULL)
		return -1;

	// Check if file open
	if (file->inUse==FALSE)
		return -1;

	// Append writes to end of file
	if (file->Append)
		file->bytenum = file->filelength;
	// Else write to current position

	// Write single byte
	Buffer[0] = (BYTE)c;
	if (_write_block(file, Buffer, 1))
		return c;
	else
		return -1;
}
#endif
//-----------------------------------------------------------------------------
// fl_fwrite: Write a block of data to the stream
//-----------------------------------------------------------------------------
#ifdef INCLUDE_WRITE_SUPPORT
int fl_fwrite(const void * data, int size, int count, FL_FILE *file )
{
	// If first call to library, initialise
	CHECK_FL_INIT();

	if (file==NULL)
		return -1;

	// Check if file open
	if (file->inUse==FALSE)
		return -1;

	// Append writes to end of file
	if (file->Append)
		file->bytenum = file->filelength;
	// Else write to current position

	if (_write_block(file, (BYTE*)data, (size*count) ))
		return count;
	else
		return -1;
}
#endif
//-----------------------------------------------------------------------------
// fl_fputs: Write a character string to the stream
//-----------------------------------------------------------------------------
#ifdef INCLUDE_WRITE_SUPPORT
int fl_fputs(const char * str, FL_FILE *file)
{
	// If first call to library, initialise
	CHECK_FL_INIT();

	if (file==NULL)
		return -1;

	// Check if file open
	if (file->inUse==FALSE)
		return -1;

	// Append writes to end of file
	if (file->Append)
		file->bytenum = file->filelength;
	// Else write to current position

	if (_write_block(file, (BYTE*)str, (UINT32)strlen(str)))
		return (int)strlen(str);
	else
		return -1;
}
#endif
//-----------------------------------------------------------------------------
// _write_block: Write a block of data to a file
//-----------------------------------------------------------------------------
#ifdef INCLUDE_WRITE_SUPPORT
static BOOL _write_block(FL_FILE *file, BYTE *data, UINT32 length) 
{
	UINT32 sector;
	UINT32 offset;	
	UINT32 i;
	BOOL dirtySector = FALSE;

	// If first call to library, initialise
	CHECK_FL_INIT();

	if (file==NULL)
		return FALSE;

	// Check if file open
	if (file->inUse==FALSE)
		return FALSE;

	// No write permissions
	if (file->Write==FALSE)
		return FALSE;

	for (i=0;i<length;i++)
	{
		// Calculations for file position
		sector = file->bytenum / 512;
		offset = file->bytenum - (sector*512);

		// If file block not already loaded
		if (file->currentBlock!=sector)
		{
			if (dirtySector)
			{
				// Copy from file buffer to FAT driver buffer
				memcpy(impl->FATFS_Internal.currentsector, file->filebuf, 512);

				// Write back current sector before loading next
				if (!FAT32_SectorWriter(file->startcluster, file->currentBlock)) 
					return FALSE;
			}

			// Read the appropriate sector
			// NOTE: This does not have succeed; if last sector of file
			// reached, no valid data will be read in, but write will 
			// allocate some more space for new data.
			FAT32_SectorReader(file->startcluster, sector);

			// Copy to file's buffer
			memcpy(file->filebuf, impl->FATFS_Internal.currentsector, 512);
			file->currentBlock=sector;
			dirtySector = FALSE;
		}

		// Get the data block
		file->filebuf[offset] = data[i];
		dirtySector = TRUE;

		// Increase next read/write position
		file->bytenum++;
	}

	// If some write data still in buffer
	if (dirtySector)
	{
		// Copy from file buffer to FAT driver buffer
		memcpy(impl->FATFS_Internal.currentsector, file->filebuf, 512);

		// Write back current sector before loading next
		if (!FAT32_SectorWriter(file->startcluster, file->currentBlock)) 
			return FALSE;
	}

	// Increase file size
	file->filelength+=length;

	// Update filesize in directory
	FAT32_UpdateFileLength(file->parentcluster, (char*)file->shortfilename, file->filelength);

	return TRUE;
}
#endif
//-----------------------------------------------------------------------------
// fl_remove: Remove a file from the filesystem
//-----------------------------------------------------------------------------
#ifdef INCLUDE_WRITE_SUPPORT
int fl_remove( const char * filename )
{
	FL_FILE* file; 
	FAT32_ShortEntry sfEntry;

	// If first call to library, initialise
	CHECK_FL_INIT();

	file = _find_spare_file();
	if (file==NULL)
		return -1;

	// Clear filename
	memset(file->path, '\n', sizeof(file->path));
	memset(file->filename, '\n', sizeof(file->filename));

	// Split full path into filename and directory path
	FileString_SplitPath((char*)filename, file->path, file->filename);

	// If file is in the root dir
	if (file->path[0]==0)
	{
		file->parentcluster = FAT32_GetRootCluster();
		file->inRoot = TRUE;
	}
	else
	{
		file->inRoot = FALSE;

		// Find parent directory start cluster
		if (!_open_directory(file->path, &file->parentcluster))
			return -1;
	}

	// Using dir cluster address search for filename
	if (FAT32_GetFileEntry(file->parentcluster, file->filename,&sfEntry))
	{
		// Initialise file details
		memcpy(file->shortfilename, sfEntry.Name, 11);
		file->filelength = sfEntry.FileSize;
		file->bytenum = 0;
		file->startcluster = (((UINT32)sfEntry.FstClusHI)<<16) + sfEntry.FstClusLO;
		file->currentBlock = 0xFFFFFFFF;

		// Delete allocated space
		if (!FAT32_FreeClusterChain(file->startcluster))
			return -1;

		// Remove directory entries
		if (!FAT32_MarkFileDeleted(file->parentcluster, (char*)file->shortfilename))
			return -1;

		FAT32_PurgeFATBuffer();
		return 0;
	}
	else
        return -1;
}
#endif



