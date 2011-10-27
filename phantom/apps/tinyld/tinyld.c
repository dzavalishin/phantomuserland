#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>

#include "elf32.h"

#include <compat/tabos.h>
#include <user/sys_fio.h>

#define NULL    0

#define TRUE	1
#define FALSE	0

//#define TINYLD_DEBUG

struct sElfRegion {
    sint32 nId;
    sint32 nStart;
    sint32 nSize;
    sint32 nVmStart;
    sint32 nVmSize;
    sint32 nFdStart;
    sint32 nFdSize;
    uint32 nDelta;
    uint32 nFlags;
};

struct sImage {
    char aName[ 256 ];
    sint32 nImageId;
    struct sImage *psNext;
    struct sImage *psPrev;
    sint32 nReferenceCount;
    uint32 nFlags;

    uint32 nEntryPoint;
    uint32 nDynamicPointer;

    uint32 *pSymHash;
    struct Elf32_Sym *psSyms;
    char *pStrTab;
    struct Elf32_Rel *psRel;
    sint32 nRelLen;
    struct Elf32_Rela *psRela;
    sint32 nRelaLen;
    struct Elf32_Rel *psPltRel;
    sint32 nPltRelLen;
    sint32 nPltRelType;
    uint32 nNumNeeded;
    struct sImage **ppsNeeded;
    uint32 nNumRegions;
    struct sElfRegion sRegions[ 1 ];
};

struct sImageQueue {
    struct sImage *psHead;
    struct sImage *psTail;
};

static struct sImageQueue sLoadedImages = { 0, 0 };
static struct sImageQueue sLoadingImages = { 0, 0 };
static uint32 nImageIdCount = 0;
static uint32 nLoadedImageCount = 0;
static char **psEnvv = NULL;

typedef void (libInitFunction)(int argc, char **argv, char **env);
typedef int (appInitFunction)(int argc, char **argv, void *arg1, void *arg2 );
typedef sint32 (dlFunc)(sint8 const *pName, uint32 nFlags );
typedef void *(dlFunc2)(sint32 nId, sint8 const *pName, uint32 nFlags );
//typedef int (appInitFunction)(int argc, char **argv, char **env, dlFunc, dlFunc2 );

#define PAGE_SIZE	4096
#define PAGE_MASK	((PAGE_SIZE)-1)
#define PAGE_BASE(y)	((y)&~(PAGE_MASK))

#define ROUNDDOWN(x,y)	((x)&~((y)-1))
#define ROUNDUP(x,y)	ROUNDDOWN(x+y-1,y)

#define STRING(image,offset)	((char*)(&(image)->pStrTab[(offset)]))
#define SYMBOL(image,num)		((struct Elf32_Sym*)&(image)->psSyms[num])
#define SYMNAME(image,sym)		STRING(image,(sym)->st_name)

#define HASHTABSIZE(image)		((image)->pSymHash[0])
#define HASHBUCKETS(image)		((uint32*)&(image)->pSymHash[2])
#define HASHCHAINS(image)		((uint32*)&(image)->pSymHash[2+HASHTABSIZE(image)])

enum {
    RFLAG_RW			= 0x0001,
    RFLAG_ANON			= 0x0002,
    RFLAG_SORTED		= 0x0400,
    RFLAG_SYMBOLIC		= 0x0800,
    RFLAG_RELOCATED		= 0x1000,
    RFLAG_PROTECTED		= 0x2000,
    RFLAG_INITIALIZED	= 0x4000
};

static void enqueueImage( struct sImageQueue *psQueue, struct sImage *psImage ) {
    psImage -> psNext = NULL;

    psImage -> psPrev = psQueue -> psTail;
    if ( psQueue -> psTail ) {
        psQueue -> psTail -> psNext = psImage;
    }
    psQueue -> psTail = psImage;
    if ( !psQueue -> psHead ) {
        psQueue -> psHead = psImage;
    }
    nLoadedImageCount++;
}

static struct sImage *findImage( char const *pName ) {
    struct sImage *psIter2;

    if ( sLoadedImages.psHead == NULL ) {
        psIter2 = NULL;
    } else {
        psIter2 = sLoadedImages.psHead;
    }

    while ( psIter2 ) {
        if ( StringCompare( psIter2 -> aName, pName ) == 0 ) {
            return psIter2;
        }
        psIter2 = psIter2 -> psNext;
    }

    psIter2 = sLoadingImages.psHead;
    while ( psIter2 ) {
        if ( StringCompare( psIter2 -> aName, pName ) == 0 ) {
            return psIter2;
        }
        psIter2 = psIter2 -> psNext;
    }
    return 0;
}

static sint32 parseElfHeader( struct Elf32_Ehdr *psElfHeader ) {
    if ( MemoryCompare( psElfHeader -> e_ident, ELF_MAGIC, 4 ) != 0 ) {
        return -EFAIL;
    }

    if ( psElfHeader -> e_ident[ 4 ] != ELFCLASS32 ) {
        return -EFAIL;
    }

    if ( psElfHeader -> e_phoff == 0 ) {
        return -EFAIL;
    }

    if ( psElfHeader -> e_phentsize < sizeof( struct Elf32_Phdr ) ) {
        return -EFAIL;
    }

    return psElfHeader -> e_phentsize * psElfHeader -> e_phnum;
}

static sint32 countRegions( char const *pBuff, sint32 nPhNum, sint32 nPhEntSize ) {
    sint32 nIndex;
    sint32 nRetVal;
    struct Elf32_Phdr *psProgHeaders;

    nRetVal = 0;

    for ( nIndex = 0; nIndex < nPhNum; nIndex++ ) {
        psProgHeaders = ( struct Elf32_Phdr * ) ( pBuff + nIndex * nPhEntSize );

        switch ( psProgHeaders -> p_type ) {
        case PT_NULL:
            /* NOP header */
            break;
        case PT_LOAD:
            nRetVal++;
            if ( psProgHeaders -> p_memsz != psProgHeaders -> p_filesz ) {
                uint32 nA = psProgHeaders -> p_vaddr + psProgHeaders -> p_memsz;
                uint32 nB = psProgHeaders -> p_vaddr + psProgHeaders -> p_filesz - 1;

                nA = PAGE_BASE( nA );
                nB = PAGE_BASE( nB );
                if ( nA != nB ) {
                    nRetVal++;
                }
            }
            break;
        case PT_DYNAMIC:
            /* will be handled at some other place */
            break;
        case PT_INTERP:
            /* should check here for appropriate interpreter */
            break;
        case PT_NOTE:
            /* unsupported */
            break;
        case PT_SHLIB:
            /* undefined semantics */
            break;
        case PT_PHDR:
            /* we don't use it */
            break;
        default:
            //Print( "Unhandled pheader type 0x%d\n", psProgHeaders[ nIndex ].p_type );
            break;
        }
    }
    return nRetVal;
}

static struct sImage *createImage( char const *pName, sint32 nNumRegions ) {
    struct sImage *psRetVal;
    sint32 nAllocSize;

    nAllocSize = sizeof( struct sImage ) + ( nNumRegions - 1 ) * sizeof( struct sElfRegion );

    psRetVal = MemoryAllocation( nAllocSize );
    MemorySet( psRetVal, 0, nAllocSize );

    StringNumCopy( psRetVal -> aName, pName, sizeof( psRetVal -> aName ) );
    psRetVal -> nImageId = nImageIdCount;
    psRetVal -> nReferenceCount = 1;
    psRetVal -> nNumRegions = nNumRegions;

    nImageIdCount++;

    return psRetVal;
}

static uint8 assertDynamicLoadable( struct sImage *psImage ) {
    uint32 nIndex;

    if ( !psImage -> nDynamicPointer ) {
        return TRUE;
    }

    for ( nIndex = 0; nIndex < psImage -> nNumRegions; nIndex++ ) {
        if ( psImage -> nDynamicPointer >= psImage -> sRegions[ nIndex ].nStart ) {
            if ( psImage -> nDynamicPointer < psImage -> sRegions[ nIndex ].nStart + psImage -> sRegions[ nIndex ].nSize ) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void parseProgramHeaders( struct sImage *psImage, char *pBuff, sint32 nPhNum, sint32 nPhEntSize ) {
    sint32 nIndex;
    sint32 nRegCount;
    struct Elf32_Phdr *psProgHeaders;

    nRegCount = 0;
    for ( nIndex = 0; nIndex < nPhNum; nIndex++ ) {
        psProgHeaders = ( struct Elf32_Phdr * ) ( pBuff + nIndex * nPhEntSize );

        switch ( psProgHeaders -> p_type ) {
        case PT_NULL:
            /* NOP header */
            break;
        case PT_LOAD:
            if ( psProgHeaders -> p_memsz == psProgHeaders -> p_filesz ) {
                /* everything in one area */
                psImage -> sRegions[ nRegCount ].nStart = psProgHeaders -> p_vaddr;
                psImage -> sRegions[ nRegCount ].nSize = psProgHeaders -> p_memsz;
                psImage -> sRegions[ nRegCount ].nVmStart = ROUNDDOWN( psProgHeaders -> p_vaddr, PAGE_SIZE );
                psImage -> sRegions[ nRegCount ].nVmSize = ROUNDUP( psProgHeaders -> p_memsz + ( psProgHeaders -> p_vaddr % PAGE_SIZE ), PAGE_SIZE );
                psImage -> sRegions[ nRegCount ].nFdStart = psProgHeaders -> p_offset;
                psImage -> sRegions[ nRegCount ].nFdSize = psProgHeaders -> p_filesz;
                psImage -> sRegions[ nRegCount ].nDelta = 0;
                psImage -> sRegions[ nRegCount ].nFlags = 0;
                if ( psProgHeaders -> p_flags & PF_W ) {
                    psImage -> sRegions[ nRegCount ].nFlags |= RFLAG_RW;
                }
            } else {
                /* may require splitting */
                uint32 nA = psProgHeaders -> p_vaddr + psProgHeaders -> p_memsz;
                uint32 nB = psProgHeaders -> p_vaddr + psProgHeaders -> p_filesz - 1;

                nA = PAGE_BASE( nA );
                nB = PAGE_BASE( nB );

                psImage -> sRegions[ nRegCount ].nStart = psProgHeaders -> p_vaddr;
                psImage -> sRegions[ nRegCount ].nSize = psProgHeaders -> p_filesz;
                psImage -> sRegions[ nRegCount ].nVmStart = ROUNDDOWN( psProgHeaders -> p_vaddr, PAGE_SIZE );
                psImage -> sRegions[ nRegCount ].nVmSize = ROUNDUP( psProgHeaders -> p_filesz + ( psProgHeaders -> p_vaddr % PAGE_SIZE ), PAGE_SIZE );
                psImage -> sRegions[ nRegCount ].nFdStart = psProgHeaders -> p_offset;
                psImage -> sRegions[ nRegCount ].nFdSize = psProgHeaders -> p_filesz;
                psImage -> sRegions[ nRegCount ].nDelta = 0;
                psImage -> sRegions[ nRegCount ].nFlags = 0;
                if ( psProgHeaders -> p_flags & PF_W ) {
                    psImage -> sRegions[ nRegCount ].nFlags |= RFLAG_RW;
                }

                if ( nA != nB ) {
                    /* yeah, it requires splitting */
                    nRegCount++;

                    psImage -> sRegions[ nRegCount ].nStart = psProgHeaders -> p_vaddr;
                    psImage -> sRegions[ nRegCount ].nSize = psProgHeaders -> p_memsz - psProgHeaders -> p_filesz;
                    psImage -> sRegions[ nRegCount ].nVmStart = psImage -> sRegions[ nRegCount - 1 ].nVmStart + psImage -> sRegions[ nRegCount - 1 ].nVmSize;
                    psImage -> sRegions[ nRegCount ].nVmSize = ROUNDUP( psProgHeaders -> p_memsz + ( psProgHeaders -> p_vaddr % PAGE_SIZE ), PAGE_SIZE ) - psImage -> sRegions[ nRegCount - 1 ].nVmSize;
                    psImage -> sRegions[ nRegCount ].nFdStart = 0;
                    psImage -> sRegions[ nRegCount ].nFdSize = 0;
                    psImage -> sRegions[ nRegCount ].nDelta = 0;
                    psImage -> sRegions[ nRegCount ].nFlags = RFLAG_ANON;
                    if ( psProgHeaders -> p_flags & PF_W ) {
                        psImage -> sRegions[ nRegCount ].nFlags |= RFLAG_RW;
                    }
                }
            }
            nRegCount++;
            break;
        case PT_DYNAMIC:
            psImage -> nDynamicPointer = psProgHeaders -> p_vaddr;
            break;
        case PT_INTERP:
            /* should check here for appropiate interpreter */
            break;
        case PT_NOTE:
            /* unsupported */
            break;
        case PT_SHLIB:
            /* undefined semantics */
            break;
        case PT_PHDR:
            /* we don't use it */
            break;
        default:
            //Print( "Unhandled pheader type 0x%x\n", psProgHeaders[ nIndex ].p_type );
            break;
        }
    }
}

enum {
    REGION_ADDR_ANY_ADDRESS = 0x0001,
    REGION_ADDR_EXACT_ADDRESS = 0x0002
};

static uint8 mapImage( sint32 nFd, char const *pPath, struct sImage *psImage, uint8 bFixed ) {
    uint32 nIndex;

    for ( nIndex = 0; nIndex < psImage -> nNumRegions; nIndex++ ) {
        sint32 nLoadAddress;
        uint32 nAddrSpecifier;

        if ( psImage -> nDynamicPointer && !bFixed ) {
            /* Relocatable image */
            if ( nIndex == 0 ) {
                nLoadAddress = 0;
                nAddrSpecifier = REGION_ADDR_ANY_ADDRESS;
            } else {
                nLoadAddress = psImage -> sRegions[ nIndex ].nVmStart + psImage -> sRegions[ nIndex - 1 ].nDelta;
                nAddrSpecifier = REGION_ADDR_EXACT_ADDRESS;
            }
        } else {
            /* Not relocatable, put it where it asks */
            nLoadAddress = psImage -> sRegions[ nIndex ].nVmStart;
            nAddrSpecifier = REGION_ADDR_EXACT_ADDRESS;
        }

        if ( psImage -> sRegions[ nIndex ].nFlags & RFLAG_ANON ) {
#ifdef TINYLD_DEBUG
            Print( "Map:RW nLoadAddress %x, Size %x, Specifier %x (1)\n",
                   nLoadAddress, psImage -> sRegions[ nIndex ].nVmSize,
                   nAddrSpecifier );
#endif
            //TODO: MAP
            nLoadAddress = VmMapAnon( nLoadAddress, nAddrSpecifier, psImage -> sRegions[ nIndex ].nVmSize );
#ifdef TINYLD_DEBUG
            Print( "nLoadAddress is %x\n", nLoadAddress );
#endif
            psImage -> sRegions[ nIndex ].nDelta = nLoadAddress - psImage -> sRegions[ nIndex ].nVmStart;
            psImage -> sRegions[ nIndex ].nVmStart = nLoadAddress;
        } else {
#ifdef TINYLD_DEBUG
            Print( "Map:RW nLoadAddress %x, Size %x, Specifier %x FdStart %x (2)\n",
                   nLoadAddress, psImage -> sRegions[ nIndex ].nVmSize,
                   nAddrSpecifier, psImage -> sRegions[ nIndex ].nFdStart );
#endif
            nLoadAddress = VmMapFile( nLoadAddress, nAddrSpecifier,
                                      psImage -> sRegions[ nIndex ].nVmSize, nFd,
                                      ROUNDDOWN( psImage -> sRegions[ nIndex ].nFdStart, PAGE_SIZE ) );
#ifdef TINYLD_DEBUG
            Print( "nLoadAddress is %x\n", nLoadAddress );
#endif
            psImage -> sRegions[ nIndex ].nDelta = nLoadAddress - psImage -> sRegions[ nIndex ].nVmStart;
            psImage -> sRegions[ nIndex ].nVmStart = nLoadAddress;
            /* Clear is not needed, because all retrieved memory is already zero'd */
            if ( psImage -> sRegions[ nIndex ].nFlags & RFLAG_RW ) {
                uint32 nStartClearing;
                uint32 nToClear;

                nStartClearing = psImage -> sRegions[ nIndex ].nVmStart +
                    ( psImage -> sRegions[ nIndex ].nStart % PAGE_SIZE )
                    + psImage -> sRegions[ nIndex ].nSize;

                nToClear = psImage -> sRegions[ nIndex ].nVmSize -
                    ( psImage -> sRegions[ nIndex ].nStart % PAGE_SIZE )
                    - psImage -> sRegions[ nIndex ].nSize;
                MemorySet( ( void * ) nStartClearing, 0, nToClear );
            }
        }
    }
    if ( psImage -> nDynamicPointer ) {
        psImage -> nDynamicPointer += psImage -> sRegions[ 0 ].nDelta;
#ifdef TINYLD_DEBUG
        Print( "DynamicPointer = %x\n", psImage -> nDynamicPointer );
#endif
    }
    return TRUE;
}

static uint8 parseDynamicSegment( struct sImage *psImage ) {
    struct Elf32_Dyn *psD;
    sint32 nIndex;

#ifdef TINYLD_DEBUG
    Print( "[%s]: psImage %x, name %s\n", __FUNCTION__, psImage, psImage -> aName );
#endif
    psImage -> pSymHash = 0;
    psImage -> psSyms = 0;
    psImage -> pStrTab = 0;

    psD = ( struct Elf32_Dyn * ) psImage -> nDynamicPointer;
    if ( !psD ) {
        return TRUE;
    }

    for ( nIndex = 0; psD[ nIndex ].d_tag != DT_NULL; nIndex++ ) {
        switch ( psD[ nIndex ].d_tag ) {
        case DT_NEEDED:
            psImage -> nNumNeeded++;
            break;
        case DT_HASH:
            psImage -> pSymHash = ( uint32 * ) ( psD[ nIndex ].d_un.d_ptr + psImage -> sRegions[ 0 ].nDelta );
            break;
        case DT_STRTAB:
            psImage -> pStrTab = ( char * ) ( psD[ nIndex ].d_un.d_ptr + psImage -> sRegions[ 0 ].nDelta );
            break;
        case DT_SYMTAB:
            psImage -> psSyms = ( struct Elf32_Sym * ) ( psD[ nIndex ].d_un.d_ptr + psImage -> sRegions[ 0 ].nDelta );
            break;
        case DT_REL:
            psImage -> psRel = ( struct Elf32_Rel * ) ( psD[ nIndex ].d_un.d_ptr + psImage -> sRegions[ 0 ].nDelta );
            break;
        case DT_RELSZ:
            psImage -> nRelLen = psD[ nIndex ].d_un.d_val;
            break;
        case DT_RELA:
            psImage -> psRela = ( struct Elf32_Rela * ) ( psD[ nIndex ].d_un.d_ptr + psImage -> sRegions[ 0 ].nDelta );
            break;
        case DT_RELASZ:
            psImage -> nRelaLen = psD[ nIndex ].d_un.d_val;
            break;
        case DT_JMPREL:
            psImage -> psPltRel = ( struct Elf32_Rel * ) ( psD[ nIndex ].d_un.d_ptr + psImage -> sRegions[ 0 ].nDelta );
            break;
        case DT_PLTRELSZ:
            psImage -> nPltRelLen = psD[ nIndex ].d_un.d_val;
            break;
        case DT_PLTREL:
            psImage -> nPltRelType = psD[ nIndex ].d_un.d_val;
            break;
        default:
            continue;
        }
    }

    if ( !psImage -> pSymHash || !psImage -> psSyms || !psImage -> pStrTab ) {
        return FALSE;
    }
    return TRUE;
}

static struct sImage *loadElf( char const *pPath, char const *pName, uint8 bFixed ) {
    sint32 nFd = -EFAIL;
    sint32 nLen;
    sint32 nPhLen;
    sint32 nIndex;
    char aPhBuff[ 4096 ];
    sint32 nNumRegions;
    struct sImage *psFound;
    struct sImage *psImage;
    struct Elf32_Ehdr sElfHeader;
    uint8 bMapSuccess;
    uint8 bDynamicSuccess;

#ifdef TINYLD_DEBUG
    Print( "[%s]: Started with pPath %s\n", __FUNCTION__, pPath );
#endif
    psFound = findImage( pName );
    if ( psFound ) {
        psFound -> nReferenceCount++;
        return psFound;
    }

    nFd = Open( ( char * ) pPath, O_RDONLY );
    if ( nFd < 0 ) {
        Print( "ERROR: Cannot open file %s\n", pPath );
        return NULL;
    }
#ifdef TINYLD_DEBUG
    Print( "nFd is %d\n", nFd );
#endif

    nLen = Read( nFd, &sElfHeader, sizeof( sElfHeader ) );
    if ( nLen != sizeof( sElfHeader ) ) {
        Print( "ERROR: Troubles reading ELF headers. Wanted %d, got %d\n",
               sizeof( sElfHeader ), nLen );
        return NULL;
    }

    nPhLen = parseElfHeader( &sElfHeader );
    if ( nPhLen <= 0 ) {
        Print( "ERROR: Incorrect ELF header\n" );
        return NULL;
    }
    if ( nPhLen > sizeof( aPhBuff ) ) {
        Print( "ERROR: Cannot handle program headers bigger then %lu\n",
               sizeof( aPhBuff ) );
        return NULL;
    }

    LongSeek( nFd, sElfHeader.e_phoff, SEEK_SET );
    nLen = Read( nFd, aPhBuff, nPhLen );
    if ( nLen != nPhLen ) {
        Print( "ERROR: Troubles reading program headers\n" );
        return NULL;
    }
    nNumRegions = countRegions( aPhBuff, sElfHeader.e_phnum, sElfHeader.e_phentsize );
    if ( nNumRegions <= 0 ) {
        Print( "ERROR: Troubles parsing program headers, nNumRegions=%d\n", nNumRegions );
        return NULL;
    }
#ifdef TINYLD_DEBUG
    Print( "nNumRegions=%d\n", nNumRegions );
#endif

    psImage = createImage( pName, nNumRegions );
    if ( psImage == NULL ) {
        Print( "ERROR: Failed to allocate image control block\n" );
        return NULL;
    }

    parseProgramHeaders( psImage, aPhBuff, sElfHeader.e_phnum, sElfHeader.e_phentsize );
    if ( !assertDynamicLoadable( psImage ) ) {
        Print( "ERROR: dynamic segment must be loadable\n" );
        return NULL;
    }

#ifdef TINYLD_DEBUG
    Print( "[%s]: Mapping file\n", __FUNCTION__ );
#endif
    bMapSuccess = mapImage( nFd, pPath, psImage, bFixed );
    if ( bMapSuccess == FALSE ) {
        Print( "ERROR: troubles reading image\n" );
        return NULL;
    }
#ifdef TINYLD_DEBUG
    Print( "[%s]: Mapping file done\n", __FUNCTION__ );
#endif

    bDynamicSuccess = parseDynamicSegment( psImage );
    if ( bDynamicSuccess == FALSE ) {
        Print( "ERROR: Troubles handling dynamic section\n" );
        return NULL;
    }

    psImage -> nEntryPoint = sElfHeader.e_entry + psImage -> sRegions[ 0 ].nDelta;
#ifdef TINYLD_DEBUG
    Print( "nEntryPoint %x\n", psImage -> nEntryPoint );
#endif

#ifdef TINYLD_DEBUG
    Print( "[%s]: path '%s', name '%s' loaded:\n",
           __FUNCTION__, pPath, pName );
#endif
    for ( nIndex = 0; nIndex < nNumRegions; nIndex++ ) {
#ifdef TINYLD_DEBUG
        Print( "\t\tid 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
               psImage -> sRegions[ nIndex ].nId,
               psImage -> sRegions[ nIndex ].nStart,
               psImage -> sRegions[ nIndex ].nSize,
               psImage -> sRegions[ nIndex ].nVmStart,
               psImage -> sRegions[ nIndex ].nVmSize,
               psImage -> sRegions[ nIndex ].nFdStart,
               psImage -> sRegions[ nIndex ].nFdSize,
               psImage -> sRegions[ nIndex ].nDelta,
               psImage -> sRegions[ nIndex ].nFlags );
#endif
    }

    Close( nFd );

    enqueueImage( &sLoadedImages, psImage );

#ifdef TINYLD_DEBUG
    Print( "[%s]: done\n", __FUNCTION__ );
#endif
    return psImage;
}

static void loadDependencies( struct sImage *psImage ) {
    uint32 nI, nJ;
    struct Elf32_Dyn *psD;
    uint32 nNeededOffset;
    char *pPath;

#ifdef TINYLD_DEBUG
    Print( "[%s]: psImage %x, name %s\n", __FUNCTION__,
           psImage, psImage -> aName );
#endif
    psD = ( struct Elf32_Dyn * ) psImage -> nDynamicPointer;
    if ( psD == NULL ) {
        return;
    }
    if ( psImage -> nNumNeeded == 0 ) {
        return;
    }

    psImage -> ppsNeeded = MemoryAllocation( psImage -> nNumNeeded * sizeof( struct sImage * ) );
    if ( psImage -> ppsNeeded == NULL ) {
#ifdef TINYLD_DEBUG
        Print( "Failed to allocate needed struct\n" );
#endif
    }

    for ( nI = 0, nJ = 0; psD[ nI ].d_tag != DT_NULL; nI++ ) {
        switch ( psD[ nI ].d_tag ) {
        case DT_NEEDED:
            nNeededOffset = psD[ nI ].d_un.d_ptr;
            pPath = MemoryAllocation( StringLength(STRING(psImage,nNeededOffset) ) + 11 );
            //StringPrint( pPath, "/boot/lib/%s", STRING(psImage,nNeededOffset) );
            StringCopy( pPath, "/boot/lib/" );
            StringCat( pPath, STRING(psImage,nNeededOffset));
#ifdef TINYLD_DEBUG
            Print( "Loading [%s], ((%s))\n", ( char * ) pPath, STRING(psImage,nNeededOffset) );
#endif
            psImage -> ppsNeeded[ nJ ] = loadElf( pPath, STRING( psImage, nNeededOffset ), FALSE );
            FreeMemory( pPath );
            nJ++;
            break;
        default:
            /* ignore any other tag */
            continue;
        }
    }

    if ( nJ != psImage -> nNumNeeded ) {
#ifdef TINYLD_DEBUG
        Print( "Internal error at [%s]\n", __FUNCTION__ );
#endif
    }
}

static uint32 elfHash( const char *pName ) {
    uint32 nHash = 0;
    uint32 nTemp;

    while ( *pName ) {
        nHash = ( nHash << 4 ) + *pName++;
        if ( ( nTemp = nHash & 0xF0000000 ) ) {
            nHash ^= nTemp >> 24;
        }
        nHash &= ~nTemp;
    }
    return nHash;
}

static struct Elf32_Sym *findSymbol( struct sImage **ppsShImg, const char *pName ) {
    struct sImage *psIter;
    uint32 nHash;
    uint32 nIndex;

    //Print( "[%s]: Searching for %s\n", __FUNCTION__, pName );
    psIter = sLoadedImages.psHead;
    while ( psIter ) {
        //Print( "[%s]: while\n", __FUNCTION__ );
        if ( psIter -> nDynamicPointer ) {
            //Print( "[%s]: computing hash\n", __FUNCTION__ );
            nHash = elfHash( pName ) % HASHTABSIZE( psIter );
            //Print( "[%s]: hash %x\n", __FUNCTION__, nHash );
            //Print( "[%s]: nIndex %x\n", __FUNCTION__, HASHBUCKETS( psIter )[ nHash ] );
            for ( nIndex = HASHBUCKETS( psIter )[ nHash ]; nIndex != STN_UNDEF; nIndex = HASHCHAINS( psIter )[ nIndex ] ) {
                if ( psIter -> psSyms[ nIndex ].st_shndx != SHN_UNDEF ) {
                    if ( ( ELF32_ST_BIND( psIter -> psSyms[ nIndex ].st_info ) == STB_GLOBAL ) || ( ELF32_ST_BIND( psIter -> psSyms[ nIndex ].st_info ) == STB_WEAK ) ) {
                        if ( !StringCompare( SYMNAME( psIter, &psIter -> psSyms[ nIndex ] ), pName ) ) {
                            *ppsShImg = psIter;
                            return &psIter -> psSyms[ nIndex ];
                        }
                    }
                }
            }
        }
        psIter = psIter -> psNext;
        //Print( "[%s]: new psIter is %x\n", __FUNCTION__, psIter );
    }
    return NULL;
}

static sint32 resolveSymbol( struct sImage *psImage, struct Elf32_Sym *psSym, uint32 *pSymAddr ) {
    struct Elf32_Sym *psSym2;
    char *pSymName;
    struct sImage *psShImg;

    switch ( psSym -> st_shndx ) {
    case SHN_UNDEF:
        pSymName = SYMNAME( psImage, psSym );

        //Print( "[%s]: Handling '%s'\n", __FUNCTION__, pSymName );
        psSym2 = findSymbol( &psShImg, pSymName );
        if ( psSym2 == NULL ) {
            Print( "Could not resolve symbol '%s'\n", pSymName );
            return -EFAIL;
        }

        /* make sure they're the same type */
        if ( ELF32_ST_TYPE( psSym -> st_info ) != STT_NOTYPE ) {
            if ( ELF32_ST_TYPE( psSym -> st_info ) != ELF32_ST_TYPE( psSym2 -> st_info ) ) {
#ifdef TINYLD_DEBUG
                Print( "found symbol '%s' in shared image but wrong type\n", pSymName );
#endif
                return -EFAIL;
            }
        }

        if ( ELF32_ST_BIND( psSym2 -> st_info ) != STB_GLOBAL && ELF32_ST_BIND( psSym2 -> st_info ) != STB_WEAK ) {
#ifdef TINYLD_DEBUG
            Print( "Found symbol '%s' but not exported\n", pSymName );
#endif
            return -EFAIL;
        }
        *pSymAddr = psSym2 -> st_value + psShImg -> sRegions[ 0 ].nDelta;
        return ESUCCESS;
    case SHN_ABS:
        *pSymAddr = psSym -> st_value + psImage -> sRegions[ 0 ].nDelta;
        return ESUCCESS;
    case SHN_COMMON:
        //#ifdef TINYLD_DEBUG
        Print( "COMMON symbol, TODO\n" );
        //#endif
        return -EFAIL;
    default:
        pSymName = SYMNAME( psImage, psSym );

        psSym2 = findSymbol( &psShImg, pSymName );
        if ( psSym2 == NULL ) {
            *pSymAddr = psSym -> st_value + psImage -> sRegions[ 0 ].nDelta;
            return ESUCCESS;
            return -EFAIL;
        }
        *pSymAddr = psSym2 -> st_value + psShImg -> sRegions[ 0 ].nDelta;
        return ESUCCESS;
    }
}

static sint32 relocateRel( struct sImage *psImage, struct Elf32_Rel *psRel, sint32 nRelLen ) {
    sint32 nIndex;
    struct Elf32_Sym *psSym;
    sint32 nVlErr;
    uint32 nS;
    uint32 nFinalVal;

#define P	((uint32*)(psImage -> sRegions[ 0 ].nDelta + psRel[ nIndex ].r_offset))
#define A	(*(P))
#define B	(psImage -> sRegions[ 0 ].nDelta )

    for ( nIndex = 0; nIndex * ( sint32 ) sizeof( struct Elf32_Rel ) < nRelLen; nIndex++ ) {
        uint32 nType = ELF32_R_TYPE( psRel[ nIndex ].r_info );

        switch ( nType ) {
        case R_386_32:
        case R_386_PC32:
        case R_386_GLOB_DAT:
        case R_386_JMP_SLOT:
        case R_386_GOTOFF:
            psSym = SYMBOL( psImage, ELF32_R_SYM( psRel[ nIndex ].r_info ) );
            nVlErr = resolveSymbol( psImage, psSym, &nS );
            if ( nVlErr < 0 ) {
                return nVlErr;
            }
        }
        switch ( nType ) {
        case R_386_NONE:
            continue;
        case R_386_32:
            nFinalVal = nS + A;
            break;
        case R_386_PC32:
            nFinalVal = nS + A - ( uint32 ) P;
            break;
        case R_386_COPY:
            continue;
        case R_386_GLOB_DAT:
            nFinalVal = nS;
            break;
        case R_386_JMP_SLOT:
            nFinalVal = nS;
            break;
        case R_386_RELATIVE:
            nFinalVal = B + A;
            break;
        default:
#ifdef TINYLD_DEBUG
            Print( "Unhandled relocation type %d\n", nType );
#endif
            return -EFAIL;
        }
        *P = nFinalVal;
    }

#undef B
#undef A
#undef P

    return ESUCCESS;
}

static uint8 relocateImage( struct sImage *psImage ) {
    sint32 nRes = ESUCCESS;

    if ( psImage -> nFlags & RFLAG_RELOCATED ) {
        return TRUE;
    }
    psImage -> nFlags |= RFLAG_RELOCATED;

    /* deal with the rels first */
    if ( psImage -> psRel ) {
        nRes = relocateRel( psImage, psImage -> psRel, psImage -> nRelLen );
        if ( nRes ) {
            return FALSE;
        }
    }

    if ( psImage -> psPltRel ) {
        nRes = relocateRel( psImage, psImage -> psPltRel, psImage -> nPltRelLen );
        if ( nRes ) {
            return FALSE;
        }
    }

    if ( psImage -> psRela ) {
#ifdef TINYLD_DEBUG
        Print( "RELA relocations not supported yet\n" );
#endif
        return FALSE;
    }
    return TRUE;
}

static uint32 topologicalSort( struct sImage *psImage, uint32 nSlot, struct sImage **ppsInitList ) {
    uint32 nIndex;

    psImage -> nFlags |= RFLAG_SORTED;
    for ( nIndex = 0; nIndex < psImage -> nNumNeeded; nIndex++ ) {
        if ( !( psImage -> ppsNeeded[ nIndex ] -> nFlags & RFLAG_SORTED ) ) {
            nSlot = topologicalSort( psImage -> ppsNeeded[ nIndex ], nSlot, ppsInitList );
        }
    }

    ppsInitList[ nSlot ] = psImage;
    return nSlot + 1;
}

static void initDependencies( struct sImage *psImage, uint8 bInitHead ) {
    uint32 nIndex;
    uint32 nSlot;
    struct sImage **ppsInitList;

    ppsInitList = MemoryAllocation( nLoadedImageCount * sizeof( struct sImage * ) );
    if ( ppsInitList == NULL ) {
#ifdef TINYLD_DEBUG
        Print( "memory shortage in [%s]\n", __FUNCTION__ );
#endif
        return;
    }

    psImage -> nFlags |= RFLAG_SORTED;
    nSlot = 0;
    for ( nIndex = 0; nIndex < psImage -> nNumNeeded; nIndex++ ) {
        if ( !( psImage -> ppsNeeded[ nIndex ] -> nFlags & RFLAG_SORTED ) ) {
            nSlot = topologicalSort( psImage -> ppsNeeded[ nIndex ], nSlot, ppsInitList );
        }
    }

    if ( bInitHead ) {
        ppsInitList[ nSlot ] = psImage;
        nSlot++;
    }

#ifdef TINYLD_DEBUG
    Print( "[%s]: nSlot %d\n", __FUNCTION__, nSlot );
#endif
    for ( nIndex = 0; nIndex < nSlot; nIndex++ ) {
        uint32 nInitFunction = ppsInitList[ nIndex ] -> nEntryPoint;
        libInitFunction *pInitFunction = ( libInitFunction * ) ( nInitFunction );

        if ( pInitFunction ) {
            pInitFunction( 0, NULL, psEnvv );
        }
    }
    FreeMemory( ppsInitList );
}

static sint32 LoadProgram( char const *pPath, uint32 *pEntry ) {
    struct sImage *psImage;
    struct sImage *psIter;

#ifdef TINYLD_DEBUG
    Print( "[%s]: pPath %s\n", __FUNCTION__, pPath );
#endif
    psImage = loadElf( pPath, pPath, TRUE );

    if ( psImage == NULL ) {
        return -1;
    }

#ifdef TINYLD_DEBUG
    Print( "[%s]: done\n", __FUNCTION__ );
#endif
    psIter = sLoadedImages.psHead;
    while ( psIter ) {
        loadDependencies( psIter );
        psIter = psIter -> psNext;
    }

    psIter = sLoadedImages.psHead;
    while ( psIter ) {
        uint8 bRelocateSuccess;

        bRelocateSuccess = relocateImage( psIter );
        if ( bRelocateSuccess == FALSE ) {
#ifdef TINYLD_DEBUG
            Print( "Troubles relocating\n" );
#endif
            return -EFAIL;
        }
        psIter = psIter -> psNext;
    }

    initDependencies( sLoadedImages.psHead, FALSE );
#ifdef TINYLD_DEBUG
    Print( "Entry point is %x\n", psImage -> nEntryPoint );
    Print( "Done\n" );
#endif

    *pEntry = psImage -> nEntryPoint;

    return psImage -> nImageId;
}

sint32 LoadLibrary( const char *pPath ) {
    struct sImage *psImage;
    struct sImage *psIter;

    if ( pPath == NULL ) {
        return -EINVAL;
    }

    psImage = findImage( pPath );
    if ( psImage ) {
        psImage -> nReferenceCount++;
        return psImage -> nImageId;
    }

    psImage = loadElf( pPath, pPath, FALSE );
    if ( psImage == NULL ) {
        return -1;
    }

    psIter = sLoadedImages.psHead;
    while ( psIter ) {
        loadDependencies( psIter );
        psIter = psIter -> psNext;
    }

    psIter = sLoadedImages.psHead;
    while ( psIter ) {
        uint8 bRelocateSuccess;

        bRelocateSuccess = relocateImage( psIter );
        if ( bRelocateSuccess == FALSE ) {
#ifdef TINYLD_DEBUG
            Print( "Troubles relocating\n" );
#endif
            return -EFAIL;
        }
        psIter = psIter -> psNext;
    }

    initDependencies( psImage, TRUE );
    return psImage -> nImageId;
}

static struct Elf32_Sym *findSymbolXxx( struct sImage *psImage, const char *pSym ) {
    uint32 nHash;
    uint32 nIndex;
    const char *pSymbol;

    pSymbol = pSym;

    if ( psImage -> nDynamicPointer ) {
        nHash = elfHash( pSymbol ) % HASHTABSIZE( psImage );
        for ( nIndex = HASHBUCKETS( psImage )[ nHash ]; nIndex != STN_UNDEF; nIndex = HASHCHAINS( psImage )[ nIndex ] ) {
            if ( psImage -> psSyms[ nIndex ].st_shndx != SHN_UNDEF ) {
                if ( ( ELF32_ST_BIND( psImage -> psSyms[ nIndex ].st_info ) == STB_GLOBAL ) || ( ELF32_ST_BIND( psImage -> psSyms[ nIndex ].st_info ) == STB_WEAK ) ) {
                    if ( !StringCompare( SYMNAME( psImage, &psImage -> psSyms[ nIndex ] ), pSymbol ) ) {
                        return &psImage -> psSyms[ nIndex ];
                    }
                }
            }
        }
    }

    return NULL;
}

static void *dynamicSymbol( sint32 nImageId, const char *pSymName ) {
    struct sImage *psIter;

    psIter = sLoadedImages.psHead;
    while ( psIter ) {
        if ( psIter -> nImageId == nImageId ) {
            struct Elf32_Sym *psSym = findSymbolXxx( psIter, pSymName );

            if ( psSym ) {
                return ( void * ) ( psSym -> st_value + psIter -> sRegions[ 0 ].nDelta );
            }
        }
        psIter = psIter -> psNext;
    }

    return NULL;
}

static sint32 internDlOpen( const char *pName, uint32 nFlags ) {
    return LoadLibrary( pName );
}

static void *internDlSym( sint32 nId, const char *pName, uint32 nFlags ) {
    return dynamicSymbol( nId, pName );
}

int main( int argc, char **argv ) {
    uint32 nEntry = 0;
    sint32 nReturnValue = -EFAIL;
    appInitFunction *pEntry;

#ifdef TINYLD_DEBUG
    /* Close stdout and stderr and set them to the debug output */
    Close( 1 );
    Close( 2 );
    Open( "/dev/tty/debug", O_RDWR );
    Open( "/dev/tty/debug", O_RDWR );
#endif

#ifdef TINYLD_DEBUG
    Print( "TinyLd: Starting up\n" );
    Print( "Argument Count %d\n", argc );
    Print( "Name: %s\n", argv[ 0 ] );
    {
        sint32 nIndex;
        for ( nIndex = 0; nIndex < argc; nIndex++ ) {
            Print( "%d. %s\n", nIndex, argv[ nIndex ] );
        }
    }
#endif
    psEnvv = NULL;//envv;
    /* argc = number of arguments for program + 1 (app name)
     * argv = arguments for program + (app name)
     */
    if ( LoadProgram( argv[ 0 ], &nEntry ) < 0 ) {
        return -1;
    }

#ifdef TINYLD_DEBUG
    Print( "[%s]: Got Entry @%x\n", __FUNCTION__, nEntry );
#endif
    if ( nEntry ) {
        pEntry = ( appInitFunction * ) nEntry;
        nReturnValue = pEntry( argc, argv, internDlOpen, internDlSym );//, psEnvv, internDlOpen, internDlSym );
    }
    //Print( "[%s]: exit: %d\n", __FUNCTION__, nReturnValue );
    ThreadExit( nReturnValue );

    return nReturnValue;
}
