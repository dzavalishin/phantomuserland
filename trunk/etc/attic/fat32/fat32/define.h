//-------------------------------------------------------------
// Global Defines
//-------------------------------------------------------------
#ifndef __DEFINE_H__
#define __DEFINE_H__

#include <phantom_types.h>

#define MAX_LONGFILENAME_ENTRIES	20

struct f32
{
    struct
    {
        // Filesystem globals
        unsigned int 		SectorsPerCluster;
        unsigned int 		cluster_begin_lba;
        unsigned int 		RootDir_First_Cluster;
        unsigned int 		fat_begin_lba;
        unsigned int 		filenumber;
        unsigned int 		fs_info_sector;
        unsigned int 		lba_begin;
        unsigned int 		fat_sectors;
    } FAT32;

    struct
    {
        unsigned char 	currentsector[512];
        unsigned int 	SectorCurrentlyLoaded; // Initially Load to 0xffffffff;
        unsigned int 	NextFreeCluster;
    } FATFS_Internal;

    struct
    {
        unsigned char	Data[512];
        unsigned int 	Sector;
        unsigned int 	Changed;
        unsigned int 	Reads;
        unsigned int 	Writes;
    } FATBuffer;

    struct
    {
        // Long File Name Structure (max 260 LFN length)
        unsigned char String[MAX_LONGFILENAME_ENTRIES][13];
        unsigned char no_of_strings;
    } FAT32_LFN;


    int (*FAT_WriteSector)(struct f32 *impl, int sector, void *buffer);
    int (*FAT_ReadSector)(struct f32 *impl, int sector, void *buffer);

    struct uufile *     dev; // IO is done via this

};

typedef struct f32 f32_t;


#define DEF_LITTLE		1
#define DEF_BIG			2

//-------------------------------------------------------------
//				   	   Compile Target
//  !!!Comment out which ever platform you are NOT using!!!
//-------------------------------------------------------------
//#define TARGET_WINDOWS			1
#define TARGET_OTHER				1

//-------------------------------------------------------------
//				   	   Target Endian
//-------------------------------------------------------------
#define TARGET_ENDIAN			DEF_LITTLE



//-------------------------------------------------------------
//					Structures/Typedefs
//-------------------------------------------------------------

#include <phantom_libc.h>
//#include <stdlib.h>
#include <string.h>

typedef unsigned char BYTE;
typedef int BOOL;
typedef u_int16_t UINT16;
typedef u_int32_t UINT32;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#define STRUCT_PACK  __attribute__ ((packed))


//-------------------------------------------------------------
//						Endian Specific
//-------------------------------------------------------------
// Little Endian
#if TARGET_ENDIAN == DEF_LITTLE
	#define GET_32BIT_WORD(buffer, location)	( ((UINT32)buffer[location+3]<<24) + ((UINT32)buffer[location+2]<<16) + ((UINT32)buffer[location+1]<<8) + (UINT32)buffer[location+0] )
	#define GET_16BIT_WORD(buffer, location)	( ((UINT16)buffer[location+1]<<8) + (UINT16)buffer[location+0] )

	#define SET_32BIT_WORD(buffer, location, value)	{ buffer[location+0] = (BYTE)((value)&0xFF); \
													  buffer[location+1] = (BYTE)((value>>8)&0xFF); \
													  buffer[location+2] = (BYTE)((value>>16)&0xFF); \
													  buffer[location+3] = (BYTE)((value>>24)&0xFF); }

	#define SET_16BIT_WORD(buffer, location, value)	{ buffer[location+0] = (BYTE)((value)&0xFF); \
													  buffer[location+1] = (BYTE)((value>>8)&0xFF); }
// Big Endian
#else
	#define GET_32BIT_WORD(buffer, location)	( ((UINT32)buffer[location+0]<<24) + ((UINT32)buffer[location+1]<<16) + ((UINT32)buffer[location+2]<<8) + (UINT32)buffer[location+3] )
	#define GET_16BIT_WORD(buffer, location)	( ((UINT16)buffer[location+0]<<8) + (UINT16)buffer[location+1] )

	#define SET_32BIT_WORD(buffer, location, value)	{ buffer[location+3] = (BYTE)((value)&0xFF); \
													  buffer[location+2] = (BYTE)((value>>8)&0xFF); \
													  buffer[location+1] = (BYTE)((value>>16)&0xFF); \
													  buffer[location+0] = (BYTE)((value>>24)&0xFF); }

	#define SET_16BIT_WORD(buffer, location, value)	{ buffer[location+1] = (BYTE)((value)&0xFF); \
													  buffer[location+0] = (BYTE)((value>>8)&0xFF); }
#endif

#endif
