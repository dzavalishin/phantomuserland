
#include "cpfs.h"
#include "cpfs_local.h"
#include "cpfs_defs.h"

#include "cpfs_test.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdarg.h>


void die_rc( const char *msg, int rc )
{
    printf("%s, rc = %d (%s) global errno = %s\n", msg, rc, strerror(rc), strerror(errno) );
    exit( 0 );
}


void test(void)
{
    // TODO tests

    test_superblock();
    test_disk_alloc();

    test_inode_blkmap(); 	// test file block allocation with inode

    // test_inode_io(); // read/write directly with inode, no file name

    // test_inode_alloc();

    test_directory();        // Create/lookup/destroy directory entries
    // test_file_create(); 	// create, open and destroy multiple files, try open deleted files
    // test_file_data();        // Create, write, close, reopen, read and compare data, in a mixed way
    // test_mutithreaded();     // Do mix of prev tests in 10 threads, starting tests in random order
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
        rc = cpfs_mkfs( 10000 );
        if( rc ) die_rc( "mkfs", rc );

        rc = cpfs_init();
        if( rc ) die_rc( "Init FS", rc );
    }

    test();

    rc = cpfs_stop();
    if( rc ) die_rc( "Stop FS", rc );


    return 0;
}


void
cpfs_panic( const char *fmt, ... )
{
    printf( "\nPanic: " );

    va_list ptr;
    va_start(ptr, fmt);
    vprintf(fmt, ptr);
    va_end(ptr);

    printf( "\nGlobal errno = %s\n\n", strerror(errno) );

    exit(33);
}


void
cpfs_log_error(char *fmt, ... )
{
    printf( "Error: ");

    va_list ptr;
    va_start(ptr, fmt);
    vprintf(fmt, ptr);
    va_end(ptr);

    printf( "\n");
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

// TODO time
cpfs_time_t
cpfs_get_current_time(void)
{
    return time(0);
}


void cpfs_mutex_lock( cpfs_mutex m)
{
}


void cpfs_mutex_unlock( cpfs_mutex m)
{
}


