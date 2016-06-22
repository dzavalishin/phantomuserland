
#include "cpfs.h"
#include "cpfs_local.h"
#include "cpfs_defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 

void die_rc( const char *msg, int rc )
{
    printf("%s, rc = %d (%s) global errno = %s\n", msg, rc, strerror(rc), strerror(errno) );
    exit( 0 );
}


void test(void)
{
}



static int dfd;

int main( int ac, char**av )
{

    errno_t 		rc;

    if(sizeof(uint64_t) < 8)
        die_rc( "int64", sizeof(uint64_t) );


    //d = open( "disk.img", O_RDWR, 0666 );
    dfd = open( "disk.img", O_RDWR|O_CREAT, 0666 );
    if( dfd < 0 ) die_rc( "open", dfd );

    rc = cpfs_init();
    if( rc )
    {
        rc = cpfs_mkfs( 8192 * 1024 );
        if( rc ) die_rc( "mkfs", rc );

        rc = cpfs_init();
        if( rc ) die_rc( "Init", rc );
    }

    test();

    rc = cpfs_stop();
    if( rc ) die_rc( "Stop", rc );


    return 0;
}


void
cpfs_panic( const char *msg )
{
    printf( "Panic: %s, global errno = %s\n", msg, strerror(errno) );
    exit(33);
}


errno_t
cpfs_disk_read( int disk_id, cpfs_blkno_t block, void *data )
{
    lseek( dfd, (int) (block*CPFS_BLKSIZE), SEEK_SET );
    int rc = read( dfd, data, CPFS_BLKSIZE );
    return (rc == CPFS_BLKSIZE) ? 0 : EIO;
}


errno_t
cpfs_disk_write( int disk_id, cpfs_blkno_t block, const void *data )
{
    lseek( dfd, (int) (block*CPFS_BLKSIZE), SEEK_SET );
    int rc = write( dfd, data, CPFS_BLKSIZE );
    return (rc == CPFS_BLKSIZE) ? 0 : EIO;
}


