/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * JetOS integration - startup and disk IO stubs.
 *
 *
**/

#include <arinc653/partition.h>


#include "cpfs.h"
#include "cpfs_local.h"
#include "cpfs_defs.h"


// ----------------------------------------------------------------------------
//
// Buffer
//
// ----------------------------------------------------------------------------


#define TEST_MEM_DISK_SZ (1*1000L*4096)

static void diskmem[TEST_MEM_DISK_SZ];
static const int   diskmem_sz = TEST_MEM_DISK_SZ; // TODO JetOS tried to make smaller disk for partition size is no more than 16m
//static int   diskmem_sz = 10*1000L*4096;





// ----------------------------------------------------------------------------
//
// OS interface functions (disk image IO)
//
// ----------------------------------------------------------------------------


static errno_t checkio( cpfs_blkno_t block, void **pos )
{
    int shift = block*CPFS_BLOCKSIZE;

    if( (shift < 0) || (shift+CPFS_BLOCKSIZE >= diskmem_sz) )
        return EIO;

    *pos = diskmem + shift;
    return 0;
}




errno_t
cpfs_disk_read( int disk_id, cpfs_blkno_t block, void *data )
{
    void *pos;
    errno_t rc = checkio( block, &pos );

    memcpy( data, pos, CPFS_BLKSIZE );
    return 0;
}


errno_t
cpfs_disk_write( int disk_id, cpfs_blkno_t block, const void *data )
{
    void *pos;
    errno_t rc = checkio( block, &pos );

    memcpy( pos, data, CPFS_BLKSIZE );
    return 0;
}

// ----------------------------------------------------------------------------
//
// ARINC Partition start
//
// ----------------------------------------------------------------------------

static int real_main(void);
static void first_process( void );


void main(void)
{
    real_main();
    STOP_SELF();
}

static int
real_main(void)
{

//    diskmem = calloc ( 1, diskmem_sz );
//	if( diskmem != 0 ) panic( "Can't calloc mem disk %dKb", diskmem_sz/1024 );

    RETURN_CODE_TYPE ret;
    PROCESS_ID_TYPE pid;
    PROCESS_ATTRIBUTE_TYPE process_attrs = {
        .PERIOD = INFINITE_TIME_VALUE,
        .TIME_CAPACITY = INFINITE_TIME_VALUE,
        .STACK_SIZE = 8096, // the only accepted stack size!
        .BASE_PRIORITY = MIN_PRIORITY_VALUE,
        .DEADLINE = SOFT,
    };

    // create process 1
    process_attrs.ENTRY_POINT = first_process;
    strncpy(process_attrs.NAME, "CPFS test process", sizeof(PROCESS_NAME_TYPE));

    CREATE_PROCESS(&process_attrs, &pid, &ret);
    if (ret != NO_ERROR) {
        printf("couldn't create process 1: %d\n", (int) ret);
        return 1;
    } else {
        printf("process 1 created\n");
    }
    
    START(pid, &ret);
    if (ret != NO_ERROR) {
        printf("couldn't start process 1: %d\n", (int) ret);
        return 1;
    } else {
        printf("process 1 \"started\" (it won't actually run until operating mode becomes NORMAL)\n");
    }


    // create ports
    //CREATE_QUEUING_PORT("QP1", 64, 10, SOURCE,      FIFO, &QP1, &ret);
    //CREATE_QUEUING_PORT("QP2", 64, 10, DESTINATION, FIFO, &QP2, &ret);


    // transition to NORMAL operating mode
    // N.B. if everything is OK, this never returns
    printf("going to NORMAL mode...\n");
    SET_PARTITION_MODE(NORMAL, &ret);

    if (ret != NO_ERROR) {
        printf("couldn't transit to normal operating mode: %d\n", (int) ret);
    } 

    STOP_SELF();
    return 0;

}


// ----------------------------------------------------------------------------
//
// Partition run
//
// ----------------------------------------------------------------------------

cpfs_fs_t fs0 =
{
    .disk_id = 0,
    .disk_size = 10000,
};



static void test_all(cpfs_fs_t *fsp)
{

    test_superblock(fsp);

    test_disk_alloc(fsp);

    test_inode_alloc(fsp);

    test_inode_blkmap(fsp); 	// test file block allocation with inode

    test_inode_io(fsp); 		// read/write directly with inode, no file name

    test_directory(fsp);        // Create/lookup/destroy directory entries

    test_file_data(fsp);        	// Create, write, close, reopen, read and compare data, in a mixed way

    test_path(fsp);

    //test_out_of_space(fsp);
}


static void
first_process( void )
{
    errno_t 		rc;

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

#if 0
    printf("\n");

    rc = cpfs_fsck( &fs0, 0 );    
    
    if( rc ) cpfs_log_error( "fsck rc=%d", rc );
#endif

    rc = cpfs_stop( &fs0 );
    if( rc ) die_rc( "Stop FS", rc );

}




// ----------------------------------------------------------------------------
//
// Util
//
// ----------------------------------------------------------------------------


void die_rc( const char *msg, int rc )
{
    printf("%s, rc = %d global errno = %d\n", msg, rc, errno );
    exit( 0 );
}



