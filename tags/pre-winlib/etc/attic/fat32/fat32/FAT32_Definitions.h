#include <phantom_types.h>

#include "unix/fat32/define.h"

#ifndef __FAT32_DEFINITIONS_H__
#define __FAT32_DEFINITIONS_H__

//#define BOOL u_int8_t
//#define BYTE u_int8_t
//#define UINT32 u_int32_t
//#define UINT16 u_int16_t

//-----------------------------------------------------------------------------
//			FAT32 Offsets
//		Name				Offset
//-----------------------------------------------------------------------------

// Boot Sector
#define BS_jmpBoot				0	// Length = 3
#define BS_OEMName				3	// Length = 8
#define BPB_BytsPerSec			11	// Length = 2
#define BPB_SecPerClus			13	// Length = 1
#define BPB_RsvdSecCnt			14	// Length = 2
#define BPB_NumFATs				16	// Length = 1
#define BPB_RootEntCnt			17	// Length = 2
#define BPB_TotSec16			19	// Length = 2
#define BPB_Media				21	// Length = 1
#define	BPB_FATSz16				22	// Length = 2
#define BPB_SecPerTrk			24	// Length = 2
#define BPB_NumHeads			26	// Length = 2
#define BPB_HiddSec				28	// Length = 4
#define BPB_TotSec32			32	// Length = 4

// FAT 12/16
#define BS_FAT_DrvNum			36	// Length = 1
#define BS_FAT_BootSig			38	// Length = 1
#define BS_FAT_VolID			39	// Length = 4
#define BS_FAT_VolLab			43	// Length = 11
#define BS_FAT_FilSysType		54	// Length = 8

// FAT 32
#define BPB_FAT32_FATSz32		36	// Length = 4
#define BPB_FAT32_ExtFlags		40	// Length = 2
#define BPB_FAT32_FSVer			42	// Length = 2
#define BPB_FAT32_RootClus		44	// Length = 4
#define BPB_FAT32_FSInfo		48	// Length = 2
#define BPB_FAT32_BkBootSec		50	// Length = 2
#define BS_FAT32_DrvNum			64	// Length = 1
#define BS_FAT32_BootSig		66	// Length = 1
#define BS_FAT32_VolID			67	// Length = 4
#define BS_FAT32_VolLab			71	// Length = 11
#define BS_FAT32_FilSysType		82	// Length = 8

//-----------------------------------------------------------------------------
// FAT Types
//-----------------------------------------------------------------------------
#define	FAT_TYPE_FAT12			1
#define	FAT_TYPE_FAT16			2
#define	FAT_TYPE_FAT32			3

//-----------------------------------------------------------------------------
// FAT32 Specific Statics
//-----------------------------------------------------------------------------
#define Signature_Position						 510
#define Signature_Value							 0xAA55
#define PARTITION1_TYPECODE_LOCATION	 		 450
#define FAT32_TYPECODE1						 	 0x0B
#define FAT32_TYPECODE2						 	 0x0C
#define PARTITION1_LBA_BEGIN_LOCATION	 		 454

//-----------------------------------------------------------------------------
// FAT32 File Attributes and Types
//-----------------------------------------------------------------------------
#define FILE_ATTR_READ_ONLY   	0x01
#define FILE_ATTR_HIDDEN 		0x02
#define FILE_ATTR_SYSTEM 		0x04
#define FILE_ATTR_SYSHID		0x06
#define FILE_ATTR_VOLUME_ID 	0x08
#define FILE_ATTR_DIRECTORY		0x10
#define FILE_ATTR_ARCHIVE  		0x20
#define FILE_ATTR_LFN_TEXT		0x0F
#define FILE_HEADER_BLANK		0x00
#define FILE_HEADER_DELETED		0xE5
#define FILE_TYPE_DIR			0x10
#define FILE_TYPE_FILE			0x20

//-----------------------------------------------------------------------------
// Other Defines
//-----------------------------------------------------------------------------
#define FAT32_EOC_FLAG			0xFFFFFFFF

#ifdef TARGET_WINDOWS
	#pragma pack(1)
#endif

typedef struct STRUCT_PACK
{
    u_int8_t 	Name[11];
    u_int8_t 	Attr;
    u_int8_t 	NTRes;
    u_int8_t 	CrtTimeTenth;
    u_int8_t 	CrtTime[2];
    u_int8_t 	CrtDate[2];
    u_int8_t 	LstAccDate[2];
    u_int16_t 	FstClusHI;
    u_int8_t 	WrtTime[2];
    u_int8_t 	WrtDate[2];
    u_int16_t 	FstClusLO;
    u_int32_t 	FileSize;
} FAT32_ShortEntry;

#endif
