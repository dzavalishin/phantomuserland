#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include <compat/tabos.h>
#include <user/sys_fio.h>

#define SECTOR_SIZE 512

void copy( char *pSrc, char *pDst ) {
	sint32 nFile1, nFile2, nLen;
	sint8 aBuffer[ SECTOR_SIZE ];

	nFile1 = Open( pDst, O_RDONLY );
	if ( nFile1 >= 0 ) {
		Print( "%s exists\n", pDst );
		Close( nFile1 );
		return;
	}
	nFile2 = Open( pSrc, O_RDONLY );
	if ( nFile2 < 0 ) {
		Print( "could not open %s (%s)\n", pSrc, StringError( nFile2 ) );
		return;
	}
	nFile1 = Open( pDst, O_WRONLY | O_CREAT );
	if ( nFile1 < 0 ) {
		Print( "could not open %s (%s)\n", pDst, StringError( nFile1 ) );
		Close( nFile2 );
		return;
	}
	while ( ( nLen = Read( nFile2, aBuffer, SECTOR_SIZE ) ) > 0 ) {
		//Print( "Read %d bytes of %s\n", nLen, pSrc );
		//Print( "Writing %d bytes to %s\n", nLen, pDst );
		nLen = Write( nFile1, aBuffer, nLen );
		if ( nLen < 0 ) {
			Print( "write failed (%s)\n", StringError( nLen ) );
			Close( nFile1 );
			Close( nFile2 );
			return;
		}
	}
	if ( nLen < 0 ) {
		Print( "read failed (%s)\n", StringError( nLen ) );
	}
	Close( nFile1 );
	Close( nFile2 );
}

int main( int argc, char **argv ) {
	if ( argc < 3 ) {
		Print( "Syntax: cp SOURCE TARGET\n" );
		Print( "        cp SOURCEs DIRECTORY\n" );
		return -EINVAL;
	}

	if ( argc == 3 ) {
		copy( argv[ 1 ], argv[ 2 ] );
	} else {
		Print( "cp: %s is a directory\n", argv[ argc - 1 ] );
		Print( "TODO!\n" );
		return -EFAIL;	    
	}
	return ESUCCESS;
}
