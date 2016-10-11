/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Test units implementation.
 *
 *
**/


#include "cpfs.h"
#include "cpfs_local.h"
#include "cpfs_defs.h"

#include "cpfs_test.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>


void    test_superblock(cpfs_fs_t *fsp)
{
    (void) fsp;
    // TODO write me
}

// ----------------------------------------------------------------------------
//
// Simple queue of block numbers used in allocation tests
//
// ----------------------------------------------------------------------------


/*
#define QSZ (2048*100)

struct tda_q {
    cpfs_blkno_t    q[QSZ];
    int             pp;
    int             gp;
};
*/

void reset_q(struct tda_q *tda)
{
    tda->pp = 0;
    tda->gp = 0;
    memset( tda->q, 0, sizeof( tda->q ) );
}


void mass_blk_alloc(cpfs_fs_t *fsp, struct tda_q *tda_q, int cnt)
{
    while( cnt-- > 0)
    {
        cpfs_blkno_t blk = cpfs_alloc_disk_block( fsp );
        if( !blk ) cpfs_panic("can't allocate block");
        if(tda_q->pp >= QSZ) cpfs_panic("test out of q");
        tda_q->q[tda_q->pp++] = blk;
    }
}

void mass_blk_free(cpfs_fs_t *fsp, struct tda_q *tda_q, int cnt)
{
    while( cnt-- > 0)
    {
        if(tda_q->gp >= QSZ) cpfs_panic("mass_blk_free test out of q");
        cpfs_blkno_t blk = tda_q->q[tda_q->gp++];
        if( !blk ) cpfs_panic("mass_blk_free blk 0");
        cpfs_free_disk_block( fsp, blk );
    }
}


// ----------------------------------------------------------------------------
//
// Test of disk block allocations and deallocations
//
// ----------------------------------------------------------------------------



void
test_disk_alloc(cpfs_fs_t *fsp)
{
    struct tda_q q;

    reset_q(&q);

    printf("Disk block allocation test: mixed alloc/free\n");
    //printf("fs.sb.free_count = %lld\n", (long long)fs.sb.free_count );

    cpfs_blkno_t initial_free = fsp->sb.free_count;

    mass_blk_alloc(fsp,&q,1);   // +
    mass_blk_alloc(fsp,&q,120); // +
#if 0 // Off for JetOS small disk tests
    mass_blk_free(fsp,&q,34);   // -
    mass_blk_alloc(fsp,&q,40);  // +
    mass_blk_free(fsp,&q,120);  // -
    mass_blk_alloc(fsp,&q,80);  // +
    mass_blk_free(fsp,&q,40);   // -
    mass_blk_alloc(fsp,&q,34);  // +
    mass_blk_free(fsp,&q,80);   // -
    mass_blk_free(fsp,&q,1);    // -

    if( initial_free != fsp->sb.free_count )
    {
        printf("FAIL: initial_free (%lld) != fs.sb.free_count (%lld)\n", (long long)initial_free, (long long)fsp->sb.free_count );
    }

    // Now do max possible run twice

    printf("Disk block allocation test: big runs\n");
    //printf("fs.sb.free_count = %lld\n", (long long)fs.sb.free_count );
    reset_q(&q);
    mass_blk_alloc(fsp,&q,1700);
    //printf("fs.sb.free_count = %lld\n", (long long)fs.sb.free_count );
    mass_blk_free(fsp,&q,1700);
    //printf("fs.sb.free_count = %lld\n", (long long)fs.sb.free_count );
    reset_q(&q);
    mass_blk_alloc(fsp,&q,1700);
    //printf("fs.sb.free_count = %lld\n", (long long)fs.sb.free_count );
    mass_blk_free(fsp,&q,1700);
    //printf("fs.sb.free_count = %lld\n", (long long)fs.sb.free_count );
#endif
    reset_q(&q);

    printf("Disk block allocation test: DONE\n");
}






// ----------------------------------------------------------------------------
//
// Test data blocks allocation for file (inode)
//
// ----------------------------------------------------------------------------





void
test_inode_blkmap(cpfs_fs_t *fsp) 	// test file block allocation with inode
{
    errno_t rc;
    cpfs_ino_t ino;
    cpfs_blkno_t phys0;
    cpfs_blkno_t phys1;

    printf("Inode block map test: allocate inode\n");
//printf( "mutex %x\n", (int)fsp->fic_mutex );
    rc = cpfs_alloc_inode( fsp, &ino );
    if( rc ) cpfs_panic( "can't alloc inode, %d", rc );

    printf("Inode block map test: inode %lld, allocate data block\n", (long long)ino);


    rc = cpfs_alloc_block_4_file( fsp, ino, 0, &phys0 );
    if( rc ) cpfs_panic( "cpfs_alloc_block_4_file rc=%d", rc );

    rc = cpfs_find_block_4_file( fsp, ino, 0, &phys1 );
    if( rc ) cpfs_panic( "cpfs_find_block_4_file rc=%d", rc );

    if( phys0 != phys1 )
        cpfs_panic( "allocated and found blocks differ: %lld and %lld", (long long)phys0, (long long)phys1 );

    if( phys0 == 0 )
        cpfs_panic( "allocated block is zero", (long long)phys0 );

    printf("Inode block map test: allocated and found blk %lld\n", (long long)phys1);

    printf("Inode block map test: free inode\n");
    cpfs_free_inode( fsp, ino );

    // todo test sparce allocation, far after the size of file

    printf("Inode block map test: DONE\n");

}










// ----------------------------------------------------------------------------
//
// Test file IO directly through inode, without using file names machinery
//
// ----------------------------------------------------------------------------






static const char test_data[] = "big brown something jumps over a lazy programmer and runs regression tests, though quite in vain";

void
test_inode_io(cpfs_fs_t *fsp) 		// read/write directly with inode, no file name
{
    errno_t rc;
    cpfs_ino_t ino;
    char test_buf[256];

    printf("Inode io test: allocate inode\n");

    rc = cpfs_alloc_inode( fsp, &ino );
    if( rc ) cpfs_panic( "can't alloc inode, %d", rc );

    printf("Inode io test: inode %lld, write file data\n", (long long)ino);

    rc = cpfs_ino_file_write( fsp, ino, 0, test_data, sizeof(test_data) );
    if( rc ) cpfs_panic( "can't write data, %d", rc );

    cpfs_blkno_t phys_blk;
    rc = cpfs_ino_file_read( fsp, ino, 0, test_buf, sizeof(test_data) , &phys_blk);
    if( rc ) cpfs_panic( "can't read data, %d", rc );

    if( memcmp( test_data, test_buf, sizeof(test_data) ) )
        cpfs_panic( "read data differs, '%s' and '%s'", test_data, test_buf );

    // TODO test read out of allocated part of file

    printf("Inode io test: DONE\n");

}






// ----------------------------------------------------------------------------
//
// Test inode alloc/free subsystem
//
// ----------------------------------------------------------------------------






void test_inode_alloc( cpfs_fs_t *fs )
{
    errno_t rc;
    cpfs_ino_t i1, i2;

    printf("Inode alloc test\n");



    rc = cpfs_alloc_inode( fs, &i1 );
    cpfs_assert( rc == 0 );

    rc = cpfs_alloc_inode( fs, &i2 );
    cpfs_assert( rc == 0 );

    //printf("Inode alloc %lld %lld\n", i1, i2 );


    rc = cpfs_free_inode( fs, i2 );
    cpfs_assert( rc == 0 );

    rc = cpfs_free_inode( fs, i1 );
    cpfs_assert( rc == 0 );



    rc = cpfs_free_inode( fs, i2 );
    cpfs_assert( rc != 0 );

    rc = cpfs_free_inode( fs, i1 );
    cpfs_assert( rc != 0 );



    printf("Inode alloc test: DONE\n");

}















// ----------------------------------------------------------------------------
//
// Test die entries creation/deletion
//
// ----------------------------------------------------------------------------






static void mke( cpfs_fs_t *fsp, const char *name )
{
    // Write inode num -1 to dir entry for it to be extremely wrong
    errno_t rc = cpfs_alloc_dirent( fsp, 0, name, 1 );
    if( rc ) cpfs_panic( "mke %d", rc );
}

static void rme( cpfs_fs_t *fsp, const char *name )
{
    //errno_t rc = cpfs_free_dirent( 0, name );
    //cpfs_ino_t ret;
    //errno_t rc = cpfs_namei( fsp, 0, name, &ret, 1 );
    errno_t rc = cpfs_free_dirent( fsp, 0, name );
    if( rc ) cpfs_panic( "rme %d", rc );
}

static void ise( cpfs_fs_t *fsp, const char *name )
{
    cpfs_ino_t ret;
    errno_t rc = cpfs_namei( fsp, 0, name, &ret );
    if( rc ) cpfs_panic( "ise %d", rc );
}

static void noe( cpfs_fs_t *fsp, const char *name )
{
    cpfs_ino_t ret;
    errno_t rc = cpfs_namei( fsp, 0, name, &ret );
    if( rc != ENOENT ) cpfs_panic( "noe %d", rc );
}


void
test_directory(cpfs_fs_t *fsp)
{
    printf("Directory entry allocation test: simpe alloc/free\n");

    mke(fsp,"f1");
    ise(fsp,"f1");
    rme(fsp,"f1");
    noe(fsp,"f1");

    printf("Directory entry allocation test: mixed alloc/free\n");

    mke(fsp,"f1");
    mke(fsp,"f2");
    ise(fsp,"f1");
    ise(fsp,"f2");

    rme(fsp,"f1");
    mke(fsp,"f3");
    noe(fsp,"f1");
    ise(fsp,"f3");

    ise(fsp,"f2");
    rme(fsp,"f2");
    noe(fsp,"f2");

    rme(fsp,"f3");
    noe(fsp,"f3");

    printf("Directory entry allocation test: DONE\n");

}












// ----------------------------------------------------------------------------
//
// Test file data IO using full cycle (open/w/r/close)
//
// ----------------------------------------------------------------------------





void
test_file_data(cpfs_fs_t *fsp)        	// Create, write, close, reopen, read and compare data, in a mixed way
{
    int fd1, fd2; //, fd3;
    errno_t rc;
    char test_buf[256];


    printf("File API data test: Create files\n");

    rc = cpfs_file_open( fsp, &fd1, "test_file_1", O_CREAT, 0 );
    if( rc )  cpfs_panic( "create 1 %d", rc );

    rc = cpfs_file_open( fsp, &fd2, "test_file_2", O_CREAT, 0 );
    if( rc )  cpfs_panic( "create 2  %d", rc );



    rc = cpfs_file_write( fd1, 0, test_data, sizeof(test_data) );
    if( rc ) cpfs_panic( "can't write data1, %d", rc );

    rc = cpfs_file_read( fd1, 0, test_buf, sizeof(test_data) );
    if( rc ) cpfs_panic( "can't read data1, %d", rc );

    if( memcmp( test_data, test_buf, sizeof(test_data) ) )
        cpfs_panic( "read data1 differs, '%s' and '%s'", test_data, test_buf );



    rc = cpfs_file_write( fd2, 100, test_data, sizeof(test_data) );
    if( rc ) cpfs_panic( "can't write data2, %d", rc );

    rc = cpfs_file_read( fd2, 100, test_buf, sizeof(test_data) );
    if( rc ) cpfs_panic( "can't read data2, %d", rc );

    if( memcmp( test_data, test_buf, sizeof(test_data) ) )
        cpfs_panic( "read data2 differs, '%s' and '%s'", test_data, test_buf );


    printf("File API data test: DONE\n");
}






// ----------------------------------------------------------------------------
//
// Test path names (sbdirs) and stat/time
//
// ----------------------------------------------------------------------------




void
test_path(cpfs_fs_t *fsp)
{
    errno_t ret;
    const char *last;
    cpfs_ino_t last_dir_ino;
    struct cpfs_stat stat;

    ret = cpfs_descend_dir( fsp, "/somefile.qq", &last, &last_dir_ino );
    test_str_eq( last, "somefile.qq" );
    test_int_eq( last_dir_ino, 0 );
    if( ret != 0 ) cpfs_panic("cpfs_descend_dir must allways succeed for single file name");

    ret = cpfs_descend_dir( fsp, "surely_nonexisting_dir/somefile.qq", &last, &last_dir_ino );
    if( ret != ENOENT ) cpfs_panic("cpfs_descend_dir found nonexisting dir");


    const char *d1 = "dir1";
    const char *d2 = "dir1/dir2";
    const char *f1 = "dir1/dir2/f1";


    // Create/remove dir

    ret = cpfs_mkdir( fsp, d1, 0 );
    test_int_eq( ret, 0 );
    //cpfs_dump_dir( fsp, 0 );

    ret = cpfs_mkdir( fsp, d1, 0 );
    test_int_eq( ret, EEXIST );

    ret = cpfs_file_unlink( fsp, d1, 0 );
    test_int_eq( ret, 0 );

    ret = cpfs_file_unlink( fsp, d1, 0 );
    test_int_eq( ret, ENOENT );




    // 2nd level dir
    ret = cpfs_mkdir( fsp, d1, 0 );
    test_int_eq( ret, 0 );
#if 1
    //cpfs_dump_dir( fsp, 0 );

    ret = cpfs_file_stat( fsp, d1, 0, &stat );
    test_int_eq( ret, 0 );

    //cpfs_dump_dir( fsp, 0 );

    cpfs_time_t now = cpfs_get_current_time();

    if( stat.ctime > now )
        cpfs_panic("ctime %lld > now %lld", stat.ctime, now);

    if( stat.ctime > stat.atime )
        cpfs_panic("ctime > atime");

    if( stat.ctime > stat.mtime )
        cpfs_panic("ctime > mtime");

    if( stat.mtime > stat.atime )
        cpfs_panic("mtime > atime");
#endif

    ret = cpfs_mkdir( fsp, d2, 0 );
    test_int_eq( ret, 0 );

#if 1
    //cpfs_dump_dir( fsp, 0 );
    ret = cpfs_file_stat( fsp, d2, 0, &stat );
    test_int_eq( ret, 0 );
    //cpfs_dump_dir( fsp, 0 );
#endif


    int fd1; //, fd2;

    ret = cpfs_file_open( fsp, &fd1, f1, O_CREAT, 0 );
    test_int_eq( ret, 0 );

    // WRONG TEST - open will open dup fn allways! impl O_EXCL?
    // Attempt to create one more with same name - must fail - TODO - doesn't fail
//    ret = cpfs_file_open( fsp, &fd2, f1, O_CREAT, 0 );
//    test_int_eq( ret, EEXIST );

#if 1
    ret = cpfs_file_stat( fsp, f1, 0, &stat );
    test_int_eq( ret, 0 );

    if( stat.ctime != stat.atime )
        cpfs_panic("ctime != atime");

    if( stat.ctime != stat.mtime )
        cpfs_panic("ctime != mtime");

    if( stat.mtime != stat.atime )
        cpfs_panic("mtime != atime");
#endif

    ret = cpfs_file_write( fd1, 0, "hello", 6 );
    test_int_eq( ret, 0 );


    ret = cpfs_file_stat( fsp, f1, 0, &stat );
    test_int_eq( ret, 0 );

    if( stat.ctime > stat.atime )
        cpfs_panic("file ctime > atime");

    if( stat.ctime > stat.mtime )
        cpfs_panic("file ctime > mtime");

    if( stat.mtime > stat.atime )
        cpfs_panic("file mtime > atime");




    ret = cpfs_file_close( fd1 );
    test_int_eq( ret, 0 );

    ret = cpfs_file_open( fsp, &fd1, f1, 0, 0 );
    test_int_eq( ret, 0 );

    ret = cpfs_file_close( fd1 );
    test_int_eq( ret, 0 );

    ret = cpfs_file_unlink( fsp, f1, 0 );
    test_int_eq( ret, 0 );

    // Remove nonempty dir
    ret = cpfs_file_unlink( fsp, d1, 0 );
    test_int_eq( ret, ENOTEMPTY );


    ret = cpfs_file_unlink( fsp, d2, 0 );
    test_int_eq( ret, 0 );

    ret = cpfs_file_unlink( fsp, d1, 0 );
    test_int_eq( ret, 0 );



    printf("Path test: DONE\n");
}




// -----------------------------------------------------------------------
//
// Out of data space and out of inodes test
//
// -----------------------------------------------------------------------


#define MAX_TEMP_FN 80

static errno_t mk_test_fn( cpfs_fs_t *fsp, char *out, int num )
{
#if 0 // if 1 will test indirection too (many files in directory, dir grows to size which needs indirection)
    (void) fsp;
    snprintf( out, MAX_TEMP_FN, "fill_file_%d", num );
#else
    errno_t rc;

    snprintf( out, MAX_TEMP_FN, "tdir%d", num/256 );
    rc = cpfs_mkdir( fsp, out, 0 );
    if( rc && (rc != EEXIST) ) return rc;
    //if( rc == ENOSPC ) return rc;
    

    snprintf( out, MAX_TEMP_FN, "tdir%d/tdir%d", num/256, num/16 );
    rc = cpfs_mkdir( fsp, out, 0 );
    if( rc && (rc != EEXIST) ) return rc;
    //if( rc == ENOSPC ) return rc;


    //snprintf( MAX_TEMP_FN, out, "tdir%d/tdir%d/fill_file_%d", num/4096, num/256, num );
    snprintf( out, MAX_TEMP_FN, "tdir%d/tdir%d/fill_file_%d", num/256, num/16, num );

    //printf( "%s\n", out );
#endif
    return 0;
}





#define MAX_FILL_SZ 4096

static errno_t fill_file( cpfs_fs_t *fsp, int num, unsigned size )
{
    errno_t ret;
    char name[MAX_TEMP_FN];
    int fd;
    const char junk[MAX_FILL_SZ];

    memset( (void *)junk, 0xAA, sizeof( junk ) ); // valgrind cries about using junk from stack

    //sprintf( name, "fill_file_%d", num );
    ret = mk_test_fn( fsp, name, num );
    if( ret ) return ret;

    ret = cpfs_file_open( fsp, &fd, name, O_CREAT, 0 );
    if( ret == ENOSPC )
        return ret;
    test_int_eq( ret, 0 );

    cpfs_size_t pos = 0;

    while( size > 0 )
    {
        unsigned part = size;
        if( part > MAX_FILL_SZ )
            part = MAX_FILL_SZ;

        ret = cpfs_file_write( fd, pos, junk, part );
        if( ret == ENOSPC )
        {
            ret = cpfs_file_close( fd );
            test_int_eq( ret, 0 );
            return ret;
        }
        test_int_eq( ret, 0 );

        pos += part;
        size -= part;
    }
        
    ret = cpfs_file_close( fd );
    test_int_eq( ret, 0 );

    return 0;
}

static errno_t kill_file( cpfs_fs_t *fsp, int num )
{
    errno_t ret;
    char name[MAX_TEMP_FN];

    //sprintf( name, "fill_file_%d", num );
    ret = mk_test_fn( fsp, name, num );
    if( ret ) return ret;

    ret = cpfs_file_unlink( fsp, name, 0 );
    //test_int_eq( ret, 0 );

    return ret;
}



void    test_out_of_space(cpfs_fs_t *fsp)
{
    errno_t ret;
    int num;
    int max = 100000;
    cpfs_blkno_t disk_size;
    cpfs_blkno_t disk_free;


    // test indirect block addrs func
    {
        errno_t 	rc;
        cpfs_blkno_t 	indexes[CPFS_MAX_INDIR] = { 0, 0, 0, 0 };
        int 		start_index = CPFS_MAX_INDIR;

        rc = calc_indirect_positions( fsp, indexes, &start_index, 4096*10 );
        cpfs_assert( !rc );
        rc = calc_indirect_positions( fsp, indexes, &start_index, 4096*11 );
        cpfs_assert( !rc );
        rc = calc_indirect_positions( fsp, indexes, &start_index, 4096*4095 );
        cpfs_assert( !rc );
        rc = calc_indirect_positions( fsp, indexes, &start_index, 0 );
        cpfs_assert( !rc );


    }


#if 1 // TODO E2BIG
    printf("Out of space test\n");
    cpfs_fs_dump( fsp );

    // Create big files, wait for ENOSPC to come (out of space)

    for( num = 0; num < max; num++ )
    {
        ret = fill_file( fsp, num, 3448*10 );
        //test_int_eq( ret, ENOSPC );
        cpfs_assert( (ret == 0) || (ret == ENOSPC) );

        if( ret == ENOSPC )
            break;
    }

    cpfs_assert( ret == ENOSPC ); // here we must make ENOSPC happen!

    max = num+1;
    printf("Out of space test: created %d files\n", max );
    cpfs_fs_dump( fsp );


    for( num = 0; num < max; num++ )
    {
        ret = kill_file( fsp, num );
        if( ret && (num = (max-1)) ) break; // last one is possibly not created
        test_int_eq( ret, 0 );
    }

    printf("Out of space test: deleted %d files\n", max );
    cpfs_fs_dump( fsp );

    cpfs_fs_stat( fsp, &disk_size, &disk_free );
    cpfs_assert( disk_size != 0 );
    cpfs_assert( disk_free != 0 );




    // Create small files, wait for ENOSPC to come (out of inodes)

    max = 100000;
    for( num = 0; num < max; num++ )
    {
        ret = fill_file( fsp, num, 2000 );
        //test_int_eq( ret, ENOSPC );
        cpfs_assert( (ret == 0) || (ret == ENOSPC) );

        if( ret == ENOSPC )
            break;
    }

    max = num+1;
    printf("Out of space test: created %d files\n", max );
    cpfs_fs_dump( fsp );


    for( num = 0; num < max; num++ )
    {
        ret = kill_file( fsp, num );
        if( ret && (num = (max-1)) ) break; // last one is possibly not created
        test_int_eq( ret, 0 );
    }

    printf("Out of space test: deleted %d files\n", max );
    cpfs_fs_dump( fsp );

    cpfs_fs_stat( fsp, &disk_size, &disk_free );
    cpfs_assert( disk_size != 0 );
    cpfs_assert( disk_free != 0 );





    printf("Out of space test: DONE\n");
#endif

}




