/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Test main. Runs FS tests in Unix/Cygwin user mode.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"
#include "cpfs_defs.h"

#include "cpfs_test.h"

#include <stdio.h>

// Don't need this file in real OS build
#ifndef __POK_LIBC_STDIO_H__

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdarg.h>

#include <pthread.h>



void die_rc( const char *msg, int rc );

static void single_test(void);
static void mt_test(void); // multithreaded test - separate FS instances
static void mp_test(void); // multithreaded test - one FS instance



// ----------------------------------------------------------------------------
//
// Definitions of two different FS instances, used in tests
//
// ----------------------------------------------------------------------------

cpfs_fs_t fs0 =
{
    .disk_id = 0,
    .disk_size = 1000, // JetOS env gives us 16 mb max, try 4mb disk
//    .disk_size = 10000,
};

cpfs_fs_t fs1 =
{
    .disk_id = 1,
    .disk_size = 12000,
};

static int dfd[2];
cpfs_fs_t *fsp_array[2] = { &fs0, &fs1 };



// ----------------------------------------------------------------------------
//
// Test main
//
// ----------------------------------------------------------------------------



int main( int ac, char**av )
{
    (void) ac;
    (void) av;

    //if(sizeof(uint64_t) < 8)
    //    die_rc( "int64", sizeof(uint64_t) );
    cpfs_assert( sizeof(uint64_t) >= 8 );


    //d = open( "disk.img", O_RDWR, 0666 );
    dfd[0] = open( "disk.img", O_RDWR|O_CREAT, 0666 );
    if( dfd[0] < 0 ) die_rc( "open", dfd[0] );

    dfd[1] = open( "disk1.img", O_RDWR|O_CREAT, 0666 );
    if( dfd[1] < 0 ) die_rc( "open", dfd[1] );



    // One thread
    single_test();

    // 2 threads, 2 disks
    //mt_test();

    // 2 threads, 1 disk
    //mp_test();


    return 0;
}


// ----------------------------------------------------------------------------
//
// OS interface functions (disk image IO)
//
// ----------------------------------------------------------------------------






errno_t
cpfs_disk_read( int disk_id, cpfs_blkno_t block, void *data )
{
    lseek( dfd[disk_id], (int) (block*CPFS_BLKSIZE), SEEK_SET );
    int rc = read( dfd[disk_id], data, CPFS_BLKSIZE );
/*
    if( TRACE ) trace(0, "%*s < cpfs_disk_read, read bytes=%d from block=%d\n", TRACE-TRACE_TAB, " ", rc, block);   
*/
    return (rc == CPFS_BLKSIZE) ? 0 : EIO;
}


errno_t
cpfs_disk_write( int disk_id, cpfs_blkno_t block, const void *data )
{
    lseek( dfd[disk_id], (int) (block*CPFS_BLKSIZE), SEEK_SET );
    int rc = write( dfd[disk_id], data, CPFS_BLKSIZE );
    return (rc == CPFS_BLKSIZE) ? 0 : EIO;
}





// ----------------------------------------------------------------------------
//
// Single thread tests
//
// ----------------------------------------------------------------------------


static void test_all(cpfs_fs_t *fsp)
{

    test_superblock(fsp);

    test_disk_alloc(fsp);

    test_inode_alloc(fsp);

    test_inode_blkmap(fsp); 	// test file block allocation with inode

    test_inode_io(fsp); 		// read/write directly with inode, no file name

    test_directory(fsp);        // Create/lookup/destroy directory entries

    // test_file_create(fsp); 	// create, open and destroy multiple files, try open deleted files

    test_file_data(fsp);        	// Create, write, close, reopen, read and compare data, in a mixed way

    // test_mutithreaded(fsp);     // Do mix of prev tests in 10 threads, starting tests in random order

    test_path(fsp);

    test_out_of_space(fsp);

}

static void
single_test(void)
{
    errno_t 		rc;


    TRACE=0;

    rc = cpfs_init( &fs0 );
    if( rc ) die_rc( "Init FS", rc );


    rc = cpfs_mount( &fs0 );
    if( rc )
    {
        rc = cpfs_mkfs( &fs0, fs0.disk_size );
        if( rc ) die_rc( "mkfs", rc );

        rc = cpfs_mount( &fs0 );
        if( rc ) die_rc( "Mount FS", rc );
    }

    test_all(&fs0);

    rc = cpfs_umount( &fs0 );
    if( rc ) die_rc( "Umount FS", rc );

#if 1
    TRACE=1;

    printf("\n");

    rc = cpfs_fsck( &fs0, 0 );    
    
    if( rc ) cpfs_log_error( "fsck rc=%d", rc );
#endif

    rc = cpfs_stop( &fs0 );
    if( rc ) die_rc( "Stop FS", rc );

}








// ----------------------------------------------------------------------------
//
// Multithreaded tests - 2 disks, 2 threads
//
// ----------------------------------------------------------------------------



// multithreaded test - separate FS instances
static void test_mt(cpfs_fs_t *fs)
{
    test_superblock(fs);

//    printf( "mutex %x\n", (int)fs->fic_mutex );
//    cpfs_mutex_lock( fs->fic_mutex );
//    cpfs_mutex_unlock( fs->fic_mutex );


    test_disk_alloc(fs);

    test_inode_blkmap(fs); 	// test file block allocation with inode

    test_inode_io(fs); 		// read/write directly with inode, no file name

    test_inode_alloc(fs);

    test_directory(fs);        // Create/lookup/destroy directory entries

    // test_file_create(fs); 	// create, open and destroy multiple files, try open deleted files

    test_file_data(fs);        	// Create, write, close, reopen, read and compare data, in a mixed way

    // test_mutithreaded(fs);     // Do mix of prev tests in 10 threads, starting tests in random order

    test_path(fs);

// [dz] temp off to work win th mp tests
    test_out_of_space(fs);

}





void* mt_run(void *arg)
{
    errno_t 		rc;
    cpfs_fs_t *		fs = arg;
    //int                 i;

    printf("Thread for disk %d run\n", fs->disk_id );

    rc = cpfs_init( fs );
    if( rc ) die_rc( "Init FS", rc );


    rc = cpfs_mount( fs );
    if( rc )
    {
        rc = cpfs_mkfs( fs, fs->disk_size );
        if( rc ) die_rc( "mkfs", rc );

        rc = cpfs_mount( fs );
        if( rc ) die_rc( "Mount FS", rc );
    }

    //for( i = 0; i < 10; i++ )
        test_mt( fs );

    rc = cpfs_umount( fs );
    if( rc ) die_rc( "Umount FS", rc );

#if 1
    printf("\n");
    rc = cpfs_fsck( fs, 0 );
    if( rc ) cpfs_log_error( "fsck rc=%d", rc );
#endif

    rc = cpfs_stop( fs );
    if( rc ) die_rc( "Stop FS", rc );

    printf("--- Thread for disk %d is DONE\n", fs->disk_id );

    return 0;
}


#define TH1 1

static void mt_test(void)
{
    int rc;
    pthread_t pt0, pt1;
    void *retval;

    printf( "\n\n--- MultiThreaded Test, different disks\n\n" );

    rc = pthread_create( &pt0, 0, mt_run, fsp_array[0] );
    if( rc ) die_rc( "thread 0", rc );

#if TH1
    rc = pthread_create( &pt1, 0, mt_run, fsp_array[1] );
    if( rc ) die_rc( "thread 1", rc );
#endif

    printf("! Wait for thread 0\n" );
    pthread_join( pt0, &retval);

#if TH1
    printf("! Wait for thread 1\n" );
    pthread_join( pt1, &retval);
#endif

    //sleep(10000);
}


















// ----------------------------------------------------------------------------
//
// Multithreaded tests - 1 disk, 2 threads
//
// ----------------------------------------------------------------------------




// multithreaded test - one FS instance
static void test_mp(cpfs_fs_t *fs)
{
    // Can do only tests that can be run cuncurrently in any combination

    // can we?

    test_mp_disk_alloc( fs );

    test_mp_inode_alloc( fs );

    //test_mp_files(fs);

}




void* mp_run(void *arg)
{
    //errno_t 		rc;
    cpfs_fs_t *		fs = arg;
    int                 i;

    printf("Thread for disk %d run (same disk)\n", fs->disk_id );

    // 10 runs is not enough
    for( i = 0; i < 50; i++ )
        test_mp( fs );

    printf("--- Thread is DONE (same disk)\n" );

    return 0;
}





static void mp_test(void)
{
    int rc;
    pthread_t pt0, pt1;
    void *retval;

    cpfs_fs_t *		fs = fsp_array[0];

    printf( "\n\n--- MultiThreaded Test, same disk\n\n" );

    rc = cpfs_init( fs );
    if( rc ) die_rc( "Init FS", rc );

    // Allways from clean disk
    //rc = cpfs_mount( fs );
    //if( rc )
    {
        rc = cpfs_mkfs( fs, fs->disk_size );
        if( rc ) die_rc( "mkfs", rc );

        rc = cpfs_mount( fs );
        if( rc ) die_rc( "Mount FS", rc );
    }

    rc = pthread_create( &pt0, 0, mp_run, fs );
    if( rc ) die_rc( "thread 0", rc );

#if TH1
    rc = pthread_create( &pt1, 0, mp_run, fs );
    if( rc ) die_rc( "thread 1", rc );
#endif

    printf("! Wait for thread 0\n" );
    pthread_join( pt0, &retval);

#if TH1
    printf("! Wait for thread 1\n" );
    pthread_join( pt1, &retval);
#endif

    rc = cpfs_umount( fs );
    if( rc ) die_rc( "Umount FS", rc );

    printf("\n");
    rc = cpfs_fsck( fs, 0 );
    if( rc ) cpfs_log_error( "fsck rc=%d", rc );

    rc = cpfs_stop( fs );
    if( rc ) die_rc( "Stop FS", rc );

    //sleep(10000);
}







// ----------------------------------------------------------------------------
//
// Util
//
// ----------------------------------------------------------------------------


void die_rc( const char *msg, int rc )
{
    printf("%s, rc = %d (%s) global errno = %s\n", msg, rc, strerror(rc), strerror(errno) );
    exit( 0 );
}




void cpfs_debug_fdump( const char *fn, void *p, unsigned size ) // dump some data to file
{
    int fd = open( fn, O_RDWR|O_CREAT, 0666 );
    if( fd < 0 )
        cpfs_panic("cpfs_debug_fdump: can't open '%d'", fn);

    if( ((int)size) != write( fd, p, size ) )
        cpfs_panic("cpfs_debug_fdump: can't write to '%d'", fn);

    close( fd );
}



#endif // POK/JetOS



