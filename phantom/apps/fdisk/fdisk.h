#ifndef FDISK_H
#define FDISK_H

#include <sys/types.h>
#include <compat/tabos.h>

struct sGeometry {
	uint32 nHeads;
	uint32 nSectors;
	uint32 nCylinders;
	uint32 nStart;
};

struct sFsTypes {
	uint8 nType;
	sint8 *pName;
};

extern struct sFsTypes asFsTypes[];

#define IDE_GET_GEOM	0x0001
#define IDE_REREAD_PART	0x0002

#define ACTIVE_FLAG		0x80
#define EXTENDED		0x05
#define WIN98_EXTENDED		0x0F
#define LINUX_EXTENDED		0x85
#define LINUX_NATIVE		0x83
#define IS_EXTENDED( i ) \
    ( ( i ) == EXTENDED || ( i ) == WIN98_EXTENDED || ( i ) == LINUX_EXTENDED )

#define NORMAL			0x00
#define CREATE_EMPTY_DOS	0x01

#define OutOfMemory		0
#define UnableToRead		1
#define UnableToWrite		2

#define SINGULAR		1
#define LINE_LENGTH		800

#define HexValue( c ) ({\
    char _c = ( c );\
    isdigit(_c) ? + _c - '0' :\
    tolower(_c) + 10 - 'a';\
})

#define SetHSC( h, s, c, sector ) {\
    s = sector % Sectors + 1;\
    sector /= Sectors;\
    h = sector % Heads;\
    sector /= Heads;\
    c = sector & 0xFF;\
    s |= ( sector >> 2 ) & 0xC0;\
}

struct part {
    uint8 Boot;		/* 0x80 - active			*/
    uint8 StartHead;	/* beginning head			*/
    uint8 StartSector;	/* beginning sector			*/
    uint8 StartCylinder;/* beginning cylinder			*/
    uint8 System;	/* partition type			*/
    uint8 EndHead;	/* end head				*/
    uint8 EndSector;	/* end sector				*/
    uint8 EndCylinder;	/* end cylinder				*/
    uint32 Start;		/* starting sector counting from 0	*/
    uint32 Blocks;	/* number of sectors in partition	*/
} __attribute__ (( packed));;

#define TableOffset(b,n)	( ( struct part * ) ( ( b ) + 0x1BE + \
    ( n ) * sizeof( struct part ) ) )


struct pte {
    struct part *Table;
    struct part *Extended;
    char Changed;
    uint32 Offset;
    char *Buffer;
} ptes[ 60 ];

void CreateDOSTable();
void GetGeometry();
void ClearPartition( struct part *p );

#endif
