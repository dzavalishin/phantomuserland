/*
 * Tabos FDISK
 * this programm is based upon the linux fdisk written by A.V. Le Blanc
 * reduced port to Tabos and adjustments by Jan-Michael Brummer
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include "fdisk.h"

#include <unistd.h>
#include <user/sys_time.h>
#include <user/sys_misc.h>

#define FDISK_DEBUG

u_int8_t RootSector[ 512 ];
char *LinePtr;
char LineBuffer[ LINE_LENGTH ];
char *DeviceName; 
int Partitions = 4;
int DeviceHandle;
uint32 ExtendedOffset = 0;
uint32 SectorSize = 512; /* 512 is the default sector size */
int DOSChanged = 0;
int ExtendedIndex;
uint32 SectorOffset = 1, Sectors, Heads, Cylinders = 1;
uint32 PartHeads, PartSectors;
uint32 KernHeads, KernSectors;
uint32 TotalNumberOfSectors;
uint32 DiskSize;

/* GetSystem()
 * gets the system id of a partition index
 *
 * params: index
 * return: system id
 */
int GetSystem( int i ) {
    return ptes[ i ].Table -> System;
}

/* Fatal()
 * handles a fatal error
 *
 * params: reason why this error happen
 * return: -
 */
void Fatal( int why ) {
    char error[ LINE_LENGTH ], *message = error;

    switch ( why ) {
	case OutOfMemory:
	    message = "Unable to allocate more memory\n";
	    break;
	case UnableToRead:
	    snprintf( error, sizeof( error ), "Unable to read %s\n", DeviceName );
	    break;
	case UnableToWrite:
	    snprintf( error, sizeof( error ), "Unable to write %s\n", DeviceName );
	    break;
	default:
	    message = "Fatal error\n";
    }
    putchar( '\n' );
    puts( message );

    ThreadExit( 1 );
}

/* TestC()
 * tests whicht message should be displayed
 *
 * params: m, message
 * return: value, 1 or 0
 */
int TestC( char **m, char *mesg ) {
    int val = 0;

    if ( !*m ) {
	printf( "You must set" );
    } else {
	printf( " %s",  *m );
	val = 1;
    }
    *m = mesg;
    return val;
}

/* PartTableFlag()
 * checks the partition table flag
 *
 * params: buffer
 * returns: sector end value
 */
uint32 PartTableFlag( char *b ) {
    return ( ( uint32 ) b[ 510 ] ) + ( ( ( uint32 ) b[ 511 ] ) << 8 );
}

/* ReReadPartitionTable()
 * sends reread command to block driver
 *
 * params: leave
 * return: -
 */
void ReReadPartitionTable( int leave ) {
    int error = 0;
    int i;

    Print( "Sending re-read partition table command.\n" );
    Sync();
    //ThreadSnooze( 200 );

    if ( ( i = IoControl( DeviceHandle, IDE_REREAD_PART, 0 ) ) != 0 ) {
	error = i;
    }

    if ( i != 0 ) {
	if ( error < 0 ) {
	    error = -error;
	}
	Print( "\nWARNING: Re-reading the partition table failed with "
	    "error %d: %s\n"
	    "The kernel still uses the old table.\n"
	    "The new table will be used at the next reboot.\n",
	    error, StringError( error ) );
    }

    if ( DOSChanged ) {
	Print( "\nWARNING: If you have created or modified any DOS 6.x\n"
	    "partitions, please see read the fdisk information.\n" );
    }

    if ( leave ) {
	Close( DeviceHandle );

	Print( "Syncing disks.\n" );
	//Sync();
	ThreadSnooze( 400 );
	ThreadExit( !!i );
    }
}

/* WarnGeometry()
 * checks and warns about missing geometry information
 *
 * params: -
 * return: 0 or 1
 */
int WarnGeometry() {
    char *m = NULL;
    int prev = 0;

    if ( !Heads ) {
	prev = TestC( &m, "Heads" );
    }
    if ( !Sectors ) {
	prev = TestC( &m, "Sectors" );
    }
    if ( !Cylinders ) {
	prev = TestC( &m, "Cylinders" );
    }

    if ( !m ) {
	return 0;
    }

    printf( "%s%s.\nYou can do this from the functions menu.\n",
	prev ? " and " : " " , m );

    return 1;
}

/* WarnCylinders()
 * warns about >1024 cylinders
 *
 * params: -
 * return: -
 */
void WarnCylinders() {
    if ( Cylinders > 1024 ) {
	printf( "\n"
	    "The number of cylinders for this disk is set to %d.\n"
	    "There is nothing wrong with that, but this is larger than 1024,\n"
	    "and could in certain setups cause problems with:\n"
	    "1) software that runs at boot time (e.g., old versiosn of LILO)\n"
	    "2) booting and partitioning software from other OSs\n"
	    "   (e.g., DOS FDISK, OS/2 FDISK)\n", Cylinders );
    }
}

/* ReadLine()
 * reads in a line from stdin
 *
 * params: -
 * return: 0 or first char
 */
int ReadLine( void ) {
	static sint32 nGotEof = 0;

	LinePtr = LineBuffer;
	if ( !FileGetString( LineBuffer, LINE_LENGTH, stdin ) ) {
		if ( FileEndOfFile( stdin ) ) {
			nGotEof++;
		}
		if ( nGotEof >= 3 ) {
			FileFlush( stdout );
			printf(  "\ngot EOF thrice - exiting..\n" );
			ThreadExit( 1 );
		}
		return 0;
	}

	while ( *LinePtr && !isgraph( *LinePtr ) ) {
		LinePtr++;
	}

	return *LinePtr;
}

/* ReadMyChar()
 * reads in a char from stdin
 *
 * params: message to display
 * return: read char
 */
char ReadMyChar( char *mesg ) {
	do {
		printf( "%s", mesg );
		//FileFlush( stdout );
	} while ( !ReadLine() );

	return *LinePtr;
}

/* ReadChars()
 * reads in chars from stdin
 *
 * params: message to display
 * return: first char
 */
char ReadChars( char *mesg ) {
	printf( "%s", mesg );
	//FileFlush( stdout );

	if ( !ReadLine() ) {
		*LinePtr = '\n';
		LinePtr[ 1 ] = 0;
	}

	return *LinePtr;
}

void ListTypes( struct sFsTypes *fs ) {
    uint32 last[ 4 ], done = 0, next = 0, size;
    int i;

    for ( i = 0; fs[ i ].pName; i++ );
    size = i;

    for ( i = 3; i >= 0; i-- ) {
	last[ 3 - i ] = done += ( size + i - done ) / ( i + 1 );
    }
    i = done = 0;

    do {
	Print( "%c%2x  %-15.15s", i ? ' ' : '\n', fs[ next ].nType, fs[ next ].pName );
	next = last[ i++ ] + done;
	if ( i > 3 || next >= last[ i ] ) {
	    i = 0;
	    next = ++done;
	}
    } while ( done < last[ 0 ] );
    Print( "\n" );
}

/* DONE */
int ReadHex( struct sFsTypes *fstypes ) {
    int hex;

    while ( 1 ) {
	ReadMyChar( "Enter hex code (L to list codes): " );
	if ( tolower( *LinePtr ) == 'l' ) {
	    ListTypes( fstypes );
	} else if ( IsHexDigit( *LinePtr ) ) {
	    hex = 0;
	    do {
		hex = hex << 4 | HexValue( *LinePtr++ );
	    } while ( IsHexDigit( *LinePtr ) );
	    return hex;
	}
    }
}

/* DONE */
uint32 ReadInteger( uint32 low, uint32 dflt, uint32 high, uint32 base, char *mesg ) {
    uint32 i;
    int defaultOk = 1;
    static char *ms = NULL;
    static int msLen = 0;

	if ( !ms || StringLength( mesg ) + 100 > msLen ) {
		msLen = StringLength( mesg ) + 200;
		//if ( !( ms = Reallocated( ms, msLen ) ) ) {
		//    Fatal( OutOfMemory );
		//}
		ms = MemoryAllocation( msLen );
		if ( !ms ) {
			Fatal( OutOfMemory );
		}
	}
	/*	msLen = StringLength( mesg ) + 200;
		//if ( !( ms = Reallocated( ms, msLen ) ) ) {
		//    Fatal( OutOfMemory );
		//}
		ms = MemoryAllocation( msLen );
		if ( !ms ) {
			Fatal( OutOfMemory );
		}*/

	if ( dflt < low || dflt > high ) {
		defaultOk = 0;
	}

	if ( defaultOk ) {
		StringNumPrint( ms, msLen, "%s (%d-%d, default %d): ", mesg, low, high, dflt );
	} else {
		StringNumPrint( ms, msLen, "%s (%d-%d): ", mesg, low, high );
	}

	while ( 1 ) {
		int useDefault = defaultOk;

		while ( ReadChars( ms ) != '\n' && !isdigit( *LinePtr ) && *LinePtr != '-' && *LinePtr != '+' ) {
			continue;
		}

		if ( *LinePtr == '+' || *LinePtr == '-' ) {
	    int minus = ( *LinePtr == '-' );
	    int absolute = 0;

	    i = StringToInteger( LinePtr + 1 );

	    while ( isdigit( *++LinePtr ) ) {
		useDefault = 0;
	    }

	    switch ( *LinePtr ) {
		case 'c':
		case 'C':
		    i *= Heads * Sectors;
		    break;
		case 'k':
		case 'K':
		    absolute = 1000;
		    break;
		case 'm':
		case 'M':
		    absolute = 1000000;
		    break;
		case 'g':
		case 'G':
		    absolute = 1000000000;
		    break;
		default:
		    break;
	    }
	    if ( absolute ) {
		unsigned long long bytes;
		uint32 unit;

		bytes = ( unsigned long long ) i * absolute;
		unit = SectorSize;
		bytes += unit / 2;
		bytes /= unit;
		i = bytes;
	    }
	    if ( minus ) {
		i = -i;
	    }
	    i += base;
	} else {
	    i = StringToInteger( LinePtr );
	    while ( isdigit( *LinePtr ) ) {
		LinePtr++;
		useDefault = 0;
	    }
	}
	if ( useDefault ) {
	    Print( "Using default value %d\n", i = dflt );
	}
	if ( i >= low && i <= high ) {
	    break;
	} else {
	    Print( "Value out of range.\n" );
	}
    }
    return i;
}

/* DONE */
void SetStartSect( struct part *p, uint32 startSect ) {
    p -> Start = startSect;
}

/* DONE */
void SetNrSects( struct part *p, uint32 nrSects ) {
    p -> Blocks = nrSects;
}

/* DONE */
uint32 GetStartSect( struct part *p ) {
    return p -> Start;
}

uint32 GetNrSects( struct part *p ) {
    return p -> Blocks;
}

/* DONE */
uint32 GetPartitionStart( struct pte *pe ) {
    return pe -> Offset + GetStartSect( pe -> Table );
}

/* DONE */
int ClearedPartition( struct part *p ) {
    return !( !p || p -> Boot || p -> StartHead || p -> StartSector ||
	p -> StartCylinder || p -> System || p -> EndHead || p -> EndSector ||
	p -> EndCylinder || GetStartSect( p ) || GetNrSects( p ) );
}

/* DONE */
int WrongOrder( int *prev ) {
    struct pte *pe;
    struct part *p;
    uint32 lastStartPos = 0, startPos;
    int i, lastI = 0;

    for ( i = 0; i < Partitions; i++ ) {
	if ( i == 4 ) {
	    lastI = 4;
	    lastStartPos = 0;
	}
	pe = &ptes[ i ];

	if ( ( p = pe -> Table ) -> System ) {
	    startPos = GetPartitionStart( pe );

	    if ( lastStartPos > startPos ) {
		if ( prev ) {
		    *prev = lastI;
		}
		return i;
	    }
	    lastStartPos = startPos;
	    lastI = i;
	}
    }
    return 0;
}

/* DONE */
int DOSPartition( int t ) {
    return ( t == 1 || t == 4 || t == 6 ||
	t == 0x0b || t == 0x0c || t == 0x0e ||
	t == 0x11 || t == 0x12 || t == 0x14 || t == 0x16 ||
	t == 0x1b || t == 0x1c || t == 0x1e || t == 0x24 ||
	t == 0xc1 || t == 0xc4 || t == 0xc6 );
}

/* DONE */
int GetPartition( int warn, int max ) {
    struct pte *pe;
    int i;

    i = ReadInteger( 1, 0, max, 0, "Select Partition number" ) - 1;
    pe = &ptes[ i ];

    if ( warn ) {
	if ( !pe -> Table -> System ) {
	    Print( "Warning: partition %d has empty type\n", i + 1 );
	}
    }
    return i;
}

/* DONE */
int GetExisitingPartition( int warn, int max ) {
    int pNo = -1;
    int i;

    for ( i = 0; i < max; i++ ) {
	struct pte *pe = &ptes[ i ];
	struct part *p = pe -> Table;

	if ( p && !ClearedPartition( p ) ) {
	    if ( pNo >= 0 ) {
		goto notUnique;
	    }
	    pNo = i;
	}
    }

    if ( pNo >= 0 ) {
	Print( "Selected partition %d\n", pNo + 1 );
	return pNo;
    }
    Print( "No partition is defined yet!\n" );
    return -1;

notUnique:
    return GetPartition( warn, max );
}

/* DONE */
int GetNonExistingPartition( int warn, int max ) {
    int pNo = - 1;
    int i;

    for ( i = 0; i < max; i++ ) {
	struct pte *pe = &ptes[ i ];
	struct part *p;

	if ( pe == NULL ) {
		continue;
	}
	p = pe -> Table;

	if ( p && ClearedPartition( p ) ) {
	    if ( pNo >= 0 ) {
		goto notUnique;
	    }
	    pNo = i;
	}
    }
    if ( pNo >= 0 ) {
	Print( "Selected partition\n" );
	return pNo;
    }
    Print( "All primary partitions have been defined already!\n" );
    return -1;

notUnique:
    return GetPartition( warn, max );
}

/* DONE */
char *StringUnits( int n ) {
    if ( n == 1 ) {
	return "sector";
    }
    return "sectors";
}

/* DONE */
void FillBounds( uint32 *first, uint32 *last ) {
    int i;
    struct pte *pe = &ptes[ 0 ];
    struct part *p;

    for ( i = 0; i < Partitions; pe++, i++ ) {
	p = pe -> Table;
	if ( !p -> System || IS_EXTENDED( p -> System ) ) {
	    first[ i ] = 0xFFFFFFFF;
	    last[ i ] = 0;
	} else {
	    first[ i ] = GetPartitionStart( pe );
	    last[ i ] = first[ i ] + GetNrSects( p ) - 1;
	}
    }
}

/* DONE */
void SetPartition( int i, int doExt, uint32 start, uint32 stop, int sysId ) {
    struct part *p;
    uint32 offset;

    if ( doExt ) {
	p = ptes[ i ].Extended;
	offset = ExtendedOffset;
    } else {
	p = ptes[ i ].Table;
	offset = ptes[ i ].Offset;
    }

    p -> Boot = 0;
    p -> System = sysId;
    SetStartSect( p, start - offset );
    SetNrSects( p, stop - start + 1 );
    if ( ( start / ( Sectors * Heads ) > 1023 ) ) {
	start = Heads * Sectors * 1024 - 1;
    }
    SetHSC( p -> StartHead, p -> StartSector, p -> StartCylinder, start );
    if ( stop / ( Sectors * Heads ) > 1023 ) {
	stop = Heads * Sectors * 1024 - 1;
    }
    SetHSC( p -> EndHead, p -> EndSector, p -> EndCylinder, stop );
    ptes[ i ].Changed = 1;
}

/* DONE */
void AddPrimary( int n, int sys ) {
    char mesg[ 256 ];
    int i, read = 0;
    struct part *p = ptes[ n ].Table;
    struct part *q = ptes[ ExtendedIndex ].Table;
    uint32 start, stop = 0, limit, temp, first[ Partitions ], last[ Partitions ];

    if ( p && p -> System ) {
	Print( "Partition %d is already present!\n", n + 1 );
	return;
    }

    FillBounds( first, last );

    if ( n < 4 ) {
	start = SectorOffset;
	limit = TotalNumberOfSectors - 1;
	if ( ExtendedOffset ) {
	    first[ ExtendedIndex ] = ExtendedOffset;
	    last[ ExtendedIndex ] = GetStartSect( q ) + GetNrSects( q ) - 1;
	}
    } else {
	start = ExtendedOffset + SectorOffset;
	limit = GetStartSect( q ) + GetNrSects( q ) - 1;
    }

    Print( "start %d, limit %d\n", start, limit );
    StringNumPrint( mesg, sizeof( mesg ), "First %s", StringUnits( SINGULAR ) );
    do {
	temp = start;

	for ( i = 0; i < Partitions; i++ ) {
	    int lastPlusOffset;

	    if ( start == ptes[ i ].Offset ) {
		start += SectorOffset;
	    }
	    lastPlusOffset = last[ i ] + ( ( n < 4 ) ? 0 : SectorOffset );
	    if ( start >= first[ i ] && start <= lastPlusOffset ) {
		start = lastPlusOffset + 1;
	    }
	}
	if ( start > limit ) {
	    break;
	}
	if ( start >= temp + 1 && read ) {
	    Print( "Sector %d is already allocated\n", temp );
	    temp = start;
	    read = 0;
	}
	if ( !read && start == temp ) {
	    uint32 i;

	    i = start;
	    start = ReadInteger( i, i, limit, 0, mesg );
	    read = 1;
	}
    } while ( start != temp || !read );

    if ( n > 4 ) {
	struct pte *pe = &ptes[ n ];

	pe -> Offset = start - SectorOffset;
	if ( pe -> Offset == ExtendedOffset ) {
	    pe -> Offset++;
	    if ( SectorOffset == 1 ) {
		start++;
	    }
	}
    }

    for ( i = 0; i < Partitions; i++ ) {
	struct pte *pe = &ptes[ i ];

	if ( start < pe -> Offset && limit >= pe -> Offset ) {
	    limit = pe -> Offset - 1;
	}
	if ( start < first[ i ] && limit >= first[ i ] ) {
	    limit = first[ i ] - 1;
	}
    }

    if ( start > limit ) {
	Print( "No free sectors available\n" );
	if ( n > 4 ) {
	    Partitions--;
	}
	return;
    }

    if ( start == limit ) {
	stop = limit;
    } else {
	StringNumPrint( mesg, sizeof( mesg ), "Last %s or +size or +sizeM or +sizeK", StringUnits( SINGULAR ) );
	stop = ReadInteger( start, limit, limit, start, mesg );
    }

    SetPartition( n, 0, start, stop, sys );

    if ( n > 4 ) {
	SetPartition( n - 1, 1, ptes[ n ].Offset, stop, EXTENDED );
    }

    if ( IS_EXTENDED( sys ) ) {
	struct pte *pe4 = &ptes[ 4 ];
	struct pte *pen = &ptes[ n ];

	ExtendedIndex = n;
	pen -> Extended = p;
	pe4 -> Offset = ExtendedOffset = start;
	if ( !( pe4 -> Buffer = CountAllocation( 1, SectorSize ) ) ) {
	    Fatal( OutOfMemory );
	}
	pe4 -> Table = TableOffset( pe4 -> Buffer, 0 );
	pe4 -> Extended = pe4 -> Table + 1;
	pe4 -> Changed = 1;
	Partitions = 5;
    }
}

/* DONE */
void AddLogical() {
    if ( Partitions > 5 || ptes[ 4 ].Table -> System ) {
	struct pte *pe = &ptes[ Partitions ];

	if ( !( pe -> Buffer = CountAllocation( 1, SectorSize ) ) ) {
	    Fatal( OutOfMemory );
	}
	pe -> Table = TableOffset( pe -> Buffer, 0 );
	pe -> Extended = pe -> Table + 1;
	pe -> Offset = 0;
	pe -> Changed = 1;
	Partitions++;
    }
    AddPrimary( Partitions - 1, LINUX_NATIVE );
}

/* DONE */
void AddPartition() {
    int i, freePrimary = 0;

    if ( WarnGeometry() ) {
	return;
    }

    for ( i = 0; i < 4; i++ ) {
	freePrimary += !ptes[ i ].Table -> System;
    }

    if ( !freePrimary && Partitions >= 60 ) {
	Print( "The maximum number of partitions has been reached\n" );
	return;
    }

    if ( !freePrimary ) {
	if ( ExtendedOffset ) {
	    AddLogical();
	} else {
	    Print( "You must delete some partitions and add an extended partition first\n" );
	}
    } else {
	char c, line[ LINE_LENGTH ];

	StringNumPrint( line, sizeof( line ),
	    "fdisk:\n%s  p primary partition (1-4): ",
	    ExtendedOffset ? "l logical (5 or over)" : "e extended" );

	while ( 1 ) {
	    if ( ( c = tolower( ReadMyChar( line ) ) ) == 'p' ) {
			int i = GetNonExistingPartition( 0, 4 );
			if ( i >= 0 ) {
				AddPrimary( i, LINUX_NATIVE );
			}
			return;
		} else if ( c == 'l' && ExtendedOffset ) {
		AddLogical();
		return;
	    } else if ( c == 'e' && !ExtendedOffset ) {
		int i = GetNonExistingPartition( 0, 4 );
		if ( i >= 0 ) {
		    AddPrimary( i, EXTENDED );
		}
		return;
	    } else {
		Print( "Invalid partition number for type '%c'\n", c );
	    }
	}
    }
}

void SetBootFlag( int i ) {
    struct pte *pe = &ptes[ i ];
    struct part *p = pe -> Table;

    if ( IS_EXTENDED( p -> System ) && !p -> Boot ) {
	printf(  "WARNING: Partition %d is an extended partition\n", i + 1 );
    }
    p -> Boot = ( p -> Boot ? 0 : ACTIVE_FLAG );
    pe -> Changed = 1;
}

void DeletePartition( int i ) {
    struct pte *pe = &ptes[ i ];
    struct part *p = pe -> Table;
    struct part *q = pe -> Extended;

    if ( WarnGeometry() ) {
	return;
    }
    pe -> Changed = 1;

    if ( i < 4 ) {
	if ( IS_EXTENDED( p -> System ) && i == ExtendedIndex ) {
	    Partitions = 4;
	    ptes[ ExtendedIndex ].Extended = NULL;
	    ExtendedOffset = 0;
	}
	ClearPartition( p );
	return;
    }

    if ( !q -> System && i > 4 ) {
	/* the last one in the chain - just delete */
	--Partitions;
	--i;
	ClearPartition( ptes[ i ].Extended );
	ptes[ i ].Changed = 1;
    } else {
	/* not the last one - further ones will be moved down */
	if ( i > 4 ) {
	    /* delete this link in the chain */
	    p = ptes[ i - 1 ].Extended;
	    *p = *q;
	    SetStartSect( p, GetStartSect( q ) );
	    SetNrSects( p, GetNrSects( q ) );
	    ptes[ i - 1 ].Changed = 1;
	} else if ( Partitions > 5 ) {
	    /* the first logical in a longer chain */
	    struct pte *pe = &ptes[ 5 ];

	    if ( pe -> Table ) {
		SetStartSect( pe -> Table, GetPartitionStart( pe ) -
		    ExtendedOffset );
	    }
	    pe -> Offset = ExtendedOffset;
	    pe -> Changed = 1;
	}

	if ( Partitions > 5 ) {
	    Partitions--;
	    while ( i < Partitions ) {
		ptes[ i ] = ptes[ i + 1 ];
		i++;
	    }
	} else {
	    ClearPartition( ptes[ i ].Table );
	}
    }
}

/* DONE */
char *PartitionType( uint8 type ) {
    int i;
    struct sFsTypes *types = ( struct sFsTypes * ) asFsTypes;

    for ( i = 0; types[ i ].pName; i++ ) {
	if ( types[ i ].nType == type ) {
	    return types[ i ].pName;
	}
    }

    return NULL;
}

int PrintPartition() {
    int i, w;
    struct part *p;
    char *type;

    w = StringLength( DeviceName );
    if ( w && isdigit( DeviceName[ w - 1 ] ) ) {
	w++;
    }

    if ( w < 5 ) {
	w = 5;
    }

    Print( "\n%s: heads: %d sectors: %d cylinders: %d, Size: %ldMB\n",
	DeviceName, Heads, Sectors, Cylinders, DiskSize );

    Print( "         Boot    Start       End    Blocks   Id  System\n" );

    for ( i = 0; i < Partitions; i++ ) {
	struct pte *pe = &ptes[ i ];

	p = pe -> Table;

	if ( p && !ClearedPartition( p ) ) {
	    uint32 pSects = GetNrSects( p );
	    uint32 pBlocks = pSects;
	    uint32 pOdd = 0;

	    if ( SectorSize < 1024 ) {
		pBlocks /= ( 1024 / SectorSize );
		pOdd = pSects % ( 1024 / SectorSize );
	    }
	    if ( SectorSize > 1024 ) {
		pBlocks *= pSects % ( 1024 / SectorSize );
	    }
	    Print( "Part%d      %c %9ld %9ld %9ld%c  %2x  %s\n", i + 1,
		!p -> Boot ? ' ' : p -> Boot == ACTIVE_FLAG ? '*' : '?',
		( long ) GetPartitionStart( pe ),
		( long ) GetPartitionStart( pe ) + pSects - ( pSects ? 1 : 0 ),
		( long ) pBlocks, pOdd ? '+' : ' ',
		p -> System,
		( type = PartitionType( p -> System ) ) ? type : "Unknown" );
	    // Check Consistency
	}
    }

    if ( WrongOrder( NULL ) ) {
	Print( "\nPartition table entries are not in disk order\n" );
    }

    return ESUCCESS;
}

/* DONE */
void SetPartitionType() {
    char *temp;
    int i, sys, origSys;
    struct part *p;

    i = GetExisitingPartition( 0, Partitions );
    if ( i == -1 ) {
	return;
    }

    p = ptes[ i ].Table;
    origSys = sys = GetSystem( i );

    if ( !sys && !GetNrSects( p ) ) {
	Print( "Partition %d does not exist yet!\n", i + 1 );
    } else while ( 1 ) {
	sys = ReadHex( asFsTypes );

	if ( !sys ) {
	    Print( "Type 0 means free space to many systems\n"
		"(but not to Linux). Having partitions of\n"
		"type 0 is probably unwise. You can delete\n"
		" a partition using the 'd' command.\n" );
	}

	if ( IS_EXTENDED( sys ) != IS_EXTENDED( p -> System ) ) {
	    Print( "You cannot change a partition into an extended one or"
		"vice versa\nDelete it first.\n" );
	    break;
	}

	if ( sys < 256 ) {
	    if ( sys == origSys ) {
		break;
	    }
	    p -> System = sys;
	    Print( "Changed system type of partition %d "
		"to %x (%s)\n", i + 1, sys,
		( temp = PartitionType( p -> System ) ) ? temp : "Unknown" );
	    ptes[ i ].Changed = 1;
	    if ( DOSPartition( sys ) ) {
		DOSChanged = 1;
	    }
	    break;
	}
    }
}

/* DONE */
void WriteSector( uint32 secNo, char *buf ) {
    int ret;

    //Print( "[%s]: secNo %d\n", __FUNCTION__, secNo );
    ret = LongSeek( DeviceHandle, secNo, SEEK_SET );
    ret = Write( DeviceHandle, buf, SectorSize );
    if (  ret != SectorSize ) {
	Fatal( UnableToWrite );
    }
}

/* DONE */
void WriteTableFlag( uint8 *b ) {
    b[ 510 ] = 0x55;
    b[ 511 ] = 0xAA;
}

/* DONE */
void WritePartitionTable() {
    //char ch;
    int i;

 	/* Write partition table */
	for ( i = 0; i < 3; i++ ) {
	    if ( ptes[ i ].Changed ) {
		ptes[ 3 ].Changed = 1;
	    }
	}
	for ( i = 3; i < Partitions; i++ ) {
	    struct pte *pe = &ptes[ i ];

	    if ( pe -> Changed ) {
		WriteTableFlag( pe -> Buffer );
		WriteSector( pe -> Offset, pe -> Buffer );
	    }
	}
    Print( "The partition table has been written\n" );
    ReReadPartitionTable( 1 );
}

/* DONE */
int ValidTableFlag( uint8 *b ) {
    return ( b[ 510 ] == 0x55 && b[ 511 ] == 0xAA );
}

/* DONE */
void ReadSector( uint32 secNo, char *buf ) {
    LongSeek( DeviceHandle, secNo, SEEK_SET );
    if ( Read( DeviceHandle, buf, SectorSize ) != SectorSize ) {
	Fatal( UnableToRead );
    }
}

/* DONE */
void ReadPTE( int pNo, uint32 offset ) {
    struct pte *pe = &ptes[ pNo ];

    pe -> Offset = offset;
    pe -> Buffer = ( char * ) MemoryAllocation( SectorSize );
    if ( !pe -> Buffer ) {
	Fatal( OutOfMemory );
    }
    ReadSector( offset, pe -> Buffer );
    pe -> Changed = 0;
    pe -> Table = pe -> Extended = NULL;
}

/* DONE */
void ClearPartition( struct part *p ) {
    if ( !p ) {
	return;
    }
    p -> Boot = 0;
    p -> StartHead = 0;
    p -> StartSector = 0;
    p -> StartCylinder = 0;
    p -> System = 0;
    p -> EndHead = 0;
    p -> EndSector = 0;
    p -> EndCylinder = 0;
    SetStartSect( p, 0 );
    SetNrSects( p, 0 );
}

void ReadExtended( int ext ) {
    int i;
    struct pte *pex;
    struct part *p, *q;

    ExtendedIndex = ext;
    pex = &ptes[ ext ];
    pex -> Extended = pex -> Table;

    p = pex -> Table;
    if ( !GetStartSect( p ) ) {
	Print( "Bad offset in primary extended partition\n" );
	return;
    }
    Print( "Start Sector is %d\n", GetStartSect( p ) );

    while ( IS_EXTENDED( p -> System ) ) {
	struct pte *pe = &ptes[ Partitions ];

	if ( Partitions >= 60 ) {
	    struct pte *pre = &ptes[ Partitions - 1 ];

	    Print( "Warning, deleting partitions after %d\n", Partitions );
	    ClearPartition( pre -> Extended );
	    pre -> Changed = 1;
	    return;
	}
	ReadPTE( Partitions, ExtendedOffset + GetStartSect( p ) );
	if ( !ExtendedOffset ) {
	    ExtendedOffset = GetStartSect( p );
	}
	q = p = TableOffset( pe -> Buffer, 0 );
	for ( i = 0; i < 4; i++, p++ ) {
	    if ( GetNrSects( p ) ) {
		if ( IS_EXTENDED( p -> System ) ) {
		    if ( pe -> Extended ) {
			Print( "Warning: extra link pointer in partition table\n" );
		    } else {
			pe -> Extended = p;
		    }
		} else if ( p -> System ) {
		    if ( pe -> Table ) {
			Print( "Warning: ignoring extra data in partition table\n" );
		    } else {
			pe -> Table = p;
		    }
		}
	    }
	}

	if ( !pe -> Table ) {
	    if ( q != pe -> Extended ) {
		pe -> Table = q;
	    } else {
		pe -> Table = q + 1;
	    }
	}
	if ( !pe -> Extended ) {
	    if ( q != pe -> Table ) {
		pe -> Extended = q;
	    } else {
		pe -> Extended = q + 1;
	    }
	}
	p = pe -> Extended;
	Partitions++;
    }
remove:
    for ( i = 4; i < Partitions; i++ ) {
	struct pte *pe = &ptes[ i ];

	if ( !GetNrSects( pe -> Table ) && ( Partitions > 5 || ptes[ 4 ].Table -> System ) ) {
	    Print( "omitting empty partition\n" );
	    //DeletePartition( i );
	    goto remove;
	}
    }
}

/* DONE */
void SetAllUnChanged() {
    int i;

    for ( i = 0; i < 60; i++ ) {
	ptes[ i ].Changed = 0;
    }
}

/* DONE */
void SetChanged( int i ) {
    ptes[ i ].Changed = 1;
}


void GetSectorSize() {
    SectorSize = 512;
}

/* DONE */
void GetKernelGeometry() {
    struct sGeometry geo;

    if ( !IoControl( DeviceHandle, IDE_GET_GEOM, ( uint32 ) &geo ) ) {
	KernHeads = geo.nHeads;
	KernSectors = geo.nSectors;
    }
    DiskSize = geo.nHeads * geo.nSectors / 2048;
}

void GetPartitionTableGeometry() {
    uint8 *bufP = RootSector;
    struct part *p;
    int i, h, s, hh, ss;
    int first = 1;
    int bad = 0;

    if ( !ValidTableFlag( bufP ) ) {
	return;
    }

    hh = ss = 0;

    for ( i = 0; i < 4; i++ ) {
	p = TableOffset( bufP, i );

	if ( p -> System != 0 ) {
	    h = p -> EndHead + 1 ;
	    s = ( p -> EndSector & 077 );
	    if ( first ) {
		hh = h;
		ss = s;
		first = 0;
	    } else if ( hh != h || ss != s ) {
		bad = 1;
	    }
	}
    }

    if ( !first && !bad ) {
	PartHeads = hh;
	PartSectors = ss;
    }
}

/* ALMOST DONE */
int ReadBootSector( int what ) {
    int i;

    Partitions = 4;

    for ( i = 0; i < 4; i++ ) {
	struct pte *pe = &ptes[ i ];

	pe -> Table = TableOffset( RootSector, i );
	pe -> Extended = NULL;
	pe -> Offset = 0;
	pe -> Buffer = RootSector;
	pe -> Changed = ( what == CREATE_EMPTY_DOS );
    }

    MemorySet( RootSector, 0, 512 );

    if ( what == CREATE_EMPTY_DOS ) {
	goto dosTable;
    }

    /* Read root sector */
    if ( Read( DeviceHandle, RootSector, 512 ) != 512 ) {
	Fatal( UnableToRead );
    }

    /* Get Geometry */
    GetGeometry();

dosTable:
    if ( !ValidTableFlag( RootSector ) ) {
	switch ( what ) {
	    case NORMAL:
		Print( "Invalid Partition Table, initializing table\n" );
		CreateDOSTable();
		return ESUCCESS;
	    case CREATE_EMPTY_DOS:
		break;
	}
    }

    WarnCylinders();
    WarnGeometry();

    for ( i = 0; i < 4; i++ ) {
	struct pte *pe = &ptes[ i ];

	if ( IS_EXTENDED( pe -> Table -> System ) ) {
	    if ( Partitions != 4 ) {
		/* IGNORE IT */
		printf(  "Ignoring extra extended partition %d\n", i + 1 );
	    } else {
		ReadExtended( i );
	    }
	}
    }

    for ( i = 3; i < Partitions; i++ ) {
	struct pte *pe = &ptes[ i ];
	if ( !ValidTableFlag( pe -> Buffer ) ) {
	    printf(  "WARNING: invalid flag 0x%04x of partitions "
		"table %d will be correcred by write\n",
		PartTableFlag( pe -> Buffer ), i + 1 );
	    // Changed
	    pe -> Changed = 1;
	}
    }

    return ESUCCESS;
}

/* DONE */
void CreateDOSTable( void ) {
    int i;

    printf( 
	"Building a new DOS disklabel. Changes will remain in memory only,\n"
	"until you decide to write them. After that, of course, the previous\n"
	"content won't be recoverable.\n\n" );
	
    Partitions = 4;

    for ( i = 510 - 64; i < 510; i++ ) {
		RootSector[ i ] = 0;
    }
    WriteTableFlag( RootSector );
    ExtendedOffset = 0;
    SetAllUnChanged();
    SetChanged( 0 );
    ReadBootSector( CREATE_EMPTY_DOS );
}

void GetGeometry() {
    int secFac;
    uint32 longSectors;

    GetSectorSize();
    secFac = SectorSize / 512;

    Heads = Cylinders = Sectors = 0;
    PartHeads = PartSectors = 0;

    GetKernelGeometry();
    GetPartitionTableGeometry();

    Heads = PartHeads ? PartHeads : KernHeads ? KernHeads : 255;
    Sectors = PartSectors ? PartSectors : KernSectors ? KernSectors : 63;
#ifdef FDISK_DEBUG
    Print( "Heads (%d/%d/%d)\n", PartHeads, KernHeads, Heads );
    Print( "Sectors (%d/%d/%d)\n", PartSectors, KernSectors, Sectors );
#endif

    longSectors = KernSectors;

    SectorOffset = 1;

    Cylinders = longSectors / ( Heads * Sectors );
    Cylinders /= secFac;

    TotalNumberOfSectors = longSectors;
}

int main( int argc, char **argp ) {
    int key, j;

    if ( argc != 2 ) {
	Print( "Syntax: fdisk [BLOCKDEVICE]\n" );
	return 1;
    }

    DeviceName = argp[ 1 ];

    DeviceHandle = Open( DeviceName, O_RDONLY );
	
    if ( DeviceHandle < 0 ) {
	Print( "can not open %s!\nexit\n", DeviceName );
	return 1;
    }

    ReadBootSector( NORMAL );

    /* Show partition table */
    PrintPartition();
    do {
	Print( "\n" );
	key = tolower( ReadMyChar( "fdisk (h=help): " ) );

	switch ( key ) {
	    case 'a':
		AddPartition();
		break;
	    case 'b':
		SetBootFlag( GetPartition( 1, Partitions ) );
		break;
	    case 'd':
		j = GetExisitingPartition( 1, Partitions );
		if ( j >= 0 ) {
		    DeletePartition( j );
		}
		break;
	    case 'h':
		Print( "a - add partition\n" );
		Print( "b - set boot flag\n" );
		Print( "d - delete partition\n" );
		Print( "p - print partition table\n" );
		Print( "t - set partition type\n" );
		Print( "q - quit fdisk\n" );
		Print( "w - write partition table\n" );
		break;
	    case 'p':
		PrintPartition();
		break;
	    case 't':
		SetPartitionType();
		break;
	    case 'w':
		WritePartitionTable();
		break;
	    default:
		break;
	}
    } while ( key != 'q' );

    Print( "\n" );

    return 0;
}
