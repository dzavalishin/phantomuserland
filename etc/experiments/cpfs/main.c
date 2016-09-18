
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
    .disk_size = 10000,
};

cpfs_fs_t fs1 =
{
    .disk_id = 1,
    .disk_size = 12000,
};

static int dfd[2];
cpfs_fs_t *fsp_array[2] = { &fs0, &fs1 };


void die_rc( const char *msg, int rc )
{
    printf("%s, rc = %d (%s) global errno = %s\n", msg, rc, strerror(rc), strerror(errno) );
    exit( 0 );
}

// ----------------------------------------------------------------------------
//
// Groups of tests
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

// multithreaded test - one FS instance
static void test_mp(cpfs_fs_t *fs) 
{
    // Can do only tests that can be run cuncurrently in any combination

    // can we?

    test_mp_disk_alloc( fs );

    test_mp_inode_alloc( fs );

    //test_mp_files(fs);

}


// ----------------------------------------------------------------------------
//
// Test main
//
// ----------------------------------------------------------------------------



int main( int ac, char**av )
{
    (void) ac;
    (void) av;

    errno_t 		rc;

    if(sizeof(uint64_t) < 8)
        die_rc( "int64", sizeof(uint64_t) );


    //d = open( "disk.img", O_RDWR, 0666 );
    dfd[0] = open( "disk.img", O_RDWR|O_CREAT, 0666 );
    if( dfd[0] < 0 ) die_rc( "open", dfd[0] );

    dfd[1] = open( "disk1.img", O_RDWR|O_CREAT, 0666 );
    if( dfd[1] < 0 ) die_rc( "open", dfd[1] );



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
    printf("\n");
    rc = cpfs_fsck( &fs0, 0 );
    if( rc ) cpfs_log_error( "fsck rc=%d", rc );
#endif

    rc = cpfs_stop( &fs0 );
    if( rc ) die_rc( "Stop FS", rc );


    mt_test();
    mp_test();


    return 0;
}


// ----------------------------------------------------------------------------
//
// OS interface functions
//
// ----------------------------------------------------------------------------



void
cpfs_panic( const char *fmt, ... )
{
    sleep(1); // let other thread to finish or die
    printf( "\n\nPanic: " );

    va_list ptr;
    va_start(ptr, fmt);
    vprintf(fmt, ptr);
    va_end(ptr);

    printf( "\nGlobal errno = %s\n\n", strerror(errno) );
#ifdef __CYGWIN__
    cygwin_stackdump();
#endif
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


cpfs_time_t
cpfs_get_current_time(void)
{
    return time(0);
}


#define USE_PTHREAD_MUTEX 1

#if !USE_PTHREAD_MUTEX
#  define MUTEX_TEST_VAL ((void *)0x3443ABBA)
#endif

void cpfs_mutex_lock( cpfs_mutex m)
{
#if USE_PTHREAD_MUTEX
    pthread_mutex_t *pm = (void *)m;

    int rc = pthread_mutex_lock( pm );
    cpfs_assert( rc == 0 );
#else
    cpfs_assert( m == MUTEX_TEST_VAL );
#endif
}


void cpfs_mutex_unlock( cpfs_mutex m)
{
#if USE_PTHREAD_MUTEX
    pthread_mutex_t *pm = (void *)m;

    int rc = pthread_mutex_unlock( pm );
    cpfs_assert( rc == 0 );
#else
    cpfs_assert( m == MUTEX_TEST_VAL );
#endif
}

void cpfs_mutex_init( cpfs_mutex *m)
{
#if USE_PTHREAD_MUTEX
    pthread_mutex_t *pm = calloc( 1, sizeof( pthread_mutex_t ) );
    cpfs_assert( pm != 0 );
    //int rc = pthread_mutex_init( pm, 0 );
    //cpfs_assert( rc == 0 );
    
    pthread_mutex_t tmp = PTHREAD_MUTEX_INITIALIZER;
    
    *pm = tmp;
    *m = (void *)pm;
#else
    *m = MUTEX_TEST_VAL;
#endif
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


errno_t
cpfs_os_access_rights_check( struct cpfs_fs *fs, cpfs_right_t t, void *user_id_data, const char *fname )
{
    (void) fs;
    (void) t;
    (void) user_id_data;
    (void) fname;

    return 0; // can do anything
}




// ----------------------------------------------------------------------------
//
// Multithreaded tests
//
// ----------------------------------------------------------------------------







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







