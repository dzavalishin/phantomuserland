/**
 *
 * Phantom OS
 *
 * Some Phantom 'FS' access code to be used in debug and cross-tools.
 *
 * (C) 2005-2008 dz.
 *
**/


#include "fsio.h"


void fsio::open( const char *fname )
{
    fs = fopen( fname, "rb+" );
}

void fsio::close()
{
    fclose( fs );
}


void fsio::read( void *buf, long block_no )
{
    fseek( fs, block_no * DISK_STRUCT_BS, SEEK_SET );

    if( 1 != fread( buf, DISK_STRUCT_BS, 1, fs ) )
        throw new fsioError("Phantom FS read error");
}


void fsio::write( void *buf, long block_no )
{
    fseek( fs, block_no * DISK_STRUCT_BS, SEEK_SET );

    if( 1 != fread( buf, DISK_STRUCT_BS, 1, fs ) )
        throw new fsioError("Phantom FS write error");
}







fslist::fslist( long _headBlock, fsio *_io, int _magic )
{
    io = _io;
    headBlock = _headBlock;
    chkMagic = _magic;

    currBlock = -1;
    posInBlock = 0;
    posInData = 0;

}


void fslist::loadPage( long blkNo )
{
    //printf("fslist::loadPage( %ld ) ", blkNo );
    io->read( &currPage, blkNo );

#if 1
    if( chkMagic != 0 && currPage.head.magic != chkMagic )
    {
        printf("bad magic 0x%X instead of 0x%X\n", currPage.head.magic, chkMagic );
        throw new fsioError("Wrong magic");
    }
#endif

#if 0
    printf("List blk magic 0x%X, used %d, next %d, first blk %d\n",
           currPage.head.magic, currPage.head.used, currPage.head.next,
           currPage.list[0]
          );
#endif
}

void fslist::loadData( long blkNo )
{
    //printf("fslist::loadData( %ld ) ", blkNo );
    io->read( &dataBuf, blkNo );
}


int fslist::read( void *_buf, int len )
{
    char *buf = (char *)_buf;    int ret = 0;

    if( currBlock == 0 )
        return 0;

    if( currBlock < 0 )
    {
        if( headBlock == 0 )
            return 0;

        loadPage( headBlock );

        if(currPage.head.used == 0)
        {
            printf("empty list\n");
            return 0;
        }

        loadData( currPage.list[0] );
        currBlock = headBlock;
        posInBlock = 1;
    }


    while( len > 0 )
    {

        if( posInData >= DISK_STRUCT_BS )
        {

            if( posInBlock >= currPage.head.used )
            {
                currBlock = currPage.head.next;
                if( currBlock == 0 ) break;
                loadPage( currBlock );
                posInBlock = 0;
            }

            //printf("pg %d ", currPage.list[posInBlock] );
            loadData( currPage.list[posInBlock++] );
            posInData = 0;
        }

        int toCopy = len;
        if(toCopy > (DISK_STRUCT_BS-posInData) )
            toCopy = DISK_STRUCT_BS-posInData;
        memcpy( buf, dataBuf+posInData, toCopy );

        len -= toCopy;
        ret += toCopy;
        buf += toCopy;
        posInData += toCopy;
    }


    return ret;
}








