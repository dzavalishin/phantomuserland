
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

#include <pthread.h>

static void mt_test(void);

cpfs_fs_t fs =
{
    .disk_id = 0,
    .disk_size = 10000,
};

cpfs_fs_t fs2 =
{
    .disk_id = 1,
    .disk_size = 10000,
};

static int dfd[2];
cpfs_fs_t *fsp[2] = { &fs, &fs2 };


void die_rc( const char *msg, int rc )
{
    printf("%s, rc = %d (%s) global errno = %s\n", msg, rc, strerror(rc), strerror(errno) );
    exit( 0 );
}


void test(void)
{
    // TODO tests

    test_path();

    test_superblock();
    test_disk_alloc();

    test_inode_blkmap(); 	// test file block allocation with inode

    test_inode_io(); 		// read/write directly with inode, no file name

    // test_inode_alloc();

    test_directory();        // Create/lookup/destroy directory entries
    // test_file_create(); 	// create, open and destroy multiple files, try open deleted files
    test_file_data();        	// Create, write, close, reopen, read and compare data, in a mixed way
    // test_mutithreaded();     // Do mix of prev tests in 10 threads, starting tests in random order

}




int main( int ac, char**av )
{

    errno_t 		rc;

    if(sizeof(uint64_t) < 8)
        die_rc( "int64", sizeof(uint64_t) );


    //d = open( "disk.img", O_RDWR, 0666 );
    dfd[0] = open( "disk.img", O_RDWR|O_CREAT, 0666 );
    if( dfd[0] < 0 ) die_rc( "open", dfd[0] );

    dfd[1] = open( "disk1.img", O_RDWR|O_CREAT, 0666 );
    if( dfd[1] < 0 ) die_rc( "open", dfd[1] );



    rc = cpfs_init( &fs );
    if( rc ) die_rc( "Init FS", rc );


    rc = cpfs_mount( &fs );
    if( rc )
    {
        rc = cpfs_mkfs( &fs, fs.disk_size );
        if( rc ) die_rc( "mkfs", rc );

        rc = cpfs_mount( &fs );
        if( rc ) die_rc( "Mount FS", rc );
    }

    test();

    rc = cpfs_umount( &fs );
    if( rc ) die_rc( "Umount FS", rc );

#if 1
    printf("\n");
    rc = cpfs_fsck( &fs, 0 );
    if( rc ) cpfs_log_error( "fsck rc=%d", rc );
#endif

    rc = cpfs_stop( &fs );
    if( rc ) die_rc( "Stop FS", rc );

    mt_test();


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
cpfs_log_error( const char *fmt, ... )
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
    lseek( dfd[disk_id], (int) (block*CPFS_BLKSIZE), SEEK_SET );
    int rc = read( dfd[disk_id], data, CPFS_BLKSIZE );
    return (rc == CPFS_BLKSIZE) ? 0 : EIO;
}


errno_t
cpfs_disk_write( int disk_id, cpfs_blkno_t block, const void *data )
{
    lseek( dfd[disk_id], (int) (block*CPFS_BLKSIZE), SEEK_SET );
    int rc = write( dfd[disk_id], data, CPFS_BLKSIZE );
    return (rc == CPFS_BLKSIZE) ? 0 : EIO;
}

// TODO time
cpfs_time_t
cpfs_get_current_time(void)
{
    return time(0);
}


#define MUTEX_TEST_VAL ((void *)0x3443ABBA)

void cpfs_mutex_lock( cpfs_mutex m)
{
    // TODO pthreads mutex?
    cpfs_assert( m == MUTEX_TEST_VAL );
}


void cpfs_mutex_unlock( cpfs_mutex m)
{
    // TODO pthreads mutex?
    cpfs_assert( m == MUTEX_TEST_VAL );
}

void cpfs_mutex_init( cpfs_mutex *m)
{
    *m = MUTEX_TEST_VAL; // temp
    // TODO pthreads mutex?
}









void cpfs_debug_fdump( const char *fn, void *p, unsigned size ) // dump some data to file
{
    int fd = open( fn, O_RDWR|O_CREAT, 0666 );
    if( fd < 0 )
        cpfs_panic("cpfs_debug_fdump: can't open '%d'", fn);

    if( size != write( fd, p, size ) )
        cpfs_panic("cpfs_debug_fdump: can't write to '%d'", fn);

    close( fd );
}




errno_t
cpfs_os_run_idle_thread( void* (*func_p)(void *arg), void *arg ) // Request OS to start thread
{
    int rc;

    pthread_attr_t a;
    pthread_t pt;

    pthread_attr_init( &a );

    // TODO idle prio!

    rc = pthread_create( &pt, &a, func_p, arg);

    pthread_attr_destroy( &a );

    return (rc == 0) ? 0 : ENOMEM;
}










void* mt_run(void *arg)
{
    errno_t 		rc;
    cpfs_fs_t *fsp = arg;

    printf("Thread for disk %d run\n", fsp->disk_id );

    rc = cpfs_init( fsp );
    if( rc ) die_rc( "Init FS", rc );


    rc = cpfs_mount( fsp );
    if( rc )
    {
        rc = cpfs_mkfs( fsp, fsp->disk_size );
        if( rc ) die_rc( "mkfs", rc );

        rc = cpfs_mount( fsp );
        if( rc ) die_rc( "Mount FS", rc );
    }

    //test();

    rc = cpfs_umount( fsp );
    if( rc ) die_rc( "Umount FS", rc );

#if 1
    printf("\n");
    rc = cpfs_fsck( fsp, 0 );
    if( rc ) cpfs_log_error( "fsck rc=%d", rc );
#endif

    rc = cpfs_stop( fsp );
    if( rc ) die_rc( "Stop FS", rc );


}




static void mt_test(void)
{
    int rc;
    pthread_t pt0, pt1;
    void *retval;

    rc = pthread_create( &pt0, 0, mt_run, fsp[0] );
    rc = pthread_create( &pt1, 0, mt_run, fsp[1] );

    pthread_join( pt0, &retval);
    pthread_join( pt1, &retval);

    //sleep(10000);
}















