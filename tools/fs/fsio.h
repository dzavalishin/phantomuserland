/**
 *
 * Phantom OS
 *
 * Some Phantom 'FS' access code to be used in debug and cross-tools.
 *
 * (C) 2005-2008 dz.
 *
**/

#include <stdio.h>
#include <string.h>
#include <phantom_disk.h>


class fsio
{
    FILE *fs;

public:
    fsio::fsio()
    {
        fs = NULL;
    }

    fsio::~fsio()
    {
        close();
    }

    void open( const char *fname );

    void close();


    void read( void *buf, long block_no );


    void write( void *buf, long block_no );




};


class fslist
{
    fsio *      io;
    long 	headBlock;
    int		chkMagic;

    long        currBlock;      // blkno of list chain block
    int         posInBlock;     // index into the list[N_REF_PER_BLOCK] - next to load, not current
    int         posInData;      // offset to the next byte to read in actual data page (dataBuf)

    phantom_disk_blocklist      currPage;

    char        dataBuf[DISK_STRUCT_BS];

    void loadPage( long blkNo );
    void loadData( long blkNo );

public:
    fslist( long _headBlock, fsio *_io, int _magic );

    int read( void *buf, int len );
};

class fsioError
{
    char *message;
public:

    fsioError(char *m) { message = strdup(m); }

    const char* getMessage() { return message; }

};


