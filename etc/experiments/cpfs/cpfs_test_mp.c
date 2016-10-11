/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Test units implementation - PARALLEL ones.
 *
 * Tests here are written to be run in parallel with each other.
 *
 *
**/


#include "cpfs.h"
#include "cpfs_local.h"
#include "cpfs_defs.h"

#include "cpfs_test.h"

#include <string.h>
#include <stdio.h>

#ifndef __POK_LIBC_STDIO_H__

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static const char test_data[] = "MP test: big brown something jumps over a lazy programmer and runs regression tests concurrently, though quite in vain";


// Unique id for file names - in these tests we need unique file names to make sure threads do not clash
static int next_index(void)
{
    static int index = 0;

    return index++;
}


void
test_mp_files(cpfs_fs_t *fs)
{

    int fd1, fd2; //, fd3;
    errno_t rc;
    char test_buf[256];

    char fn1[32];
    char fn2[32];

    sprintf( fn1, "test_file_mp_%d", next_index() );
    sprintf( fn2, "test_file_mp_%d", next_index() );

    printf("MP File API data test: Create files\n");



    rc = cpfs_file_open( fs, &fd1, fn1, O_CREAT, 0 );
    if( rc )  cpfs_panic( "create 1 rc=%d", rc );

    rc = cpfs_file_open( fs, &fd2, fn2, O_CREAT, 0 );
    if( rc )  cpfs_panic( "create 2 rc=%d", rc );



    rc = cpfs_file_write( fd1, 0, test_data, sizeof(test_data) );
    if( rc ) cpfs_panic( "can't write data1, %d", rc );

    rc = cpfs_file_read( fd1, 0, test_buf, sizeof(test_data) );
    if( rc ) cpfs_panic( "can't read data1, %d", rc );

    if( memcmp( test_data, test_buf, sizeof(test_data) ) )
        cpfs_panic( "read data1 differs, '%s' and '%s'", test_data, test_buf );



    rc = cpfs_file_write( fd2, 100, test_data, sizeof(test_data) );
    if( rc ) cpfs_panic( "can't write data2, rc=%d", rc );

    rc = cpfs_file_read( fd2, 100, test_buf, sizeof(test_data) );
    if( rc ) cpfs_panic( "can't read data2, rc=%d", rc );

    if( memcmp( test_data, test_buf, sizeof(test_data) ) )
        cpfs_panic( "read data2 differs, '%s' and '%s'", test_data, test_buf );



    rc = cpfs_file_close( fd1 );
    cpfs_assert( rc == 0 );

    rc = cpfs_file_close( fd2 );
    cpfs_assert( rc == 0 );



    printf("MP File API data test: DONE\n");


}








void
test_mp_disk_alloc(cpfs_fs_t *fsp)
{
    struct tda_q q;

    reset_q(&q);

    printf("MP Disk block allocation test: mixed alloc/free\n");
    //printf("fs.sb.free_count = %lld\n", (long long)fs.sb.free_count );

    //cpfs_blkno_t initial_free = fsp->sb.free_count;

    mass_blk_alloc(fsp,&q,1);   // +
    mass_blk_alloc(fsp,&q,120); // +
    mass_blk_free(fsp,&q,34);   // -
    mass_blk_alloc(fsp,&q,40);  // +
    mass_blk_free(fsp,&q,120);  // -
    mass_blk_alloc(fsp,&q,80);  // +
    mass_blk_free(fsp,&q,40);   // -
    mass_blk_alloc(fsp,&q,34);  // +
    mass_blk_free(fsp,&q,80);   // -
    mass_blk_free(fsp,&q,1);    // -

/*
    if( initial_free != fsp->sb.free_count )
    {
        printf("FAIL: initial_free (%lld) != fs.sb.free_count (%lld)\n", (long long)initial_free, (long long)fsp->sb.free_count );
    }
*/
    // Now do max possible run twice

    printf("MP Disk block allocation test: big runs\n");
    //printf("fs.sb.free_count = %lld\n", (long long)fs.sb.free_count );
    reset_q(&q);
    mass_blk_alloc(fsp,&q,800);
    //printf("fs.sb.free_count = %lld\n", (long long)fs.sb.free_count );
    mass_blk_free(fsp,&q,800);
    //printf("fs.sb.free_count = %lld\n", (long long)fs.sb.free_count );
    reset_q(&q);
    mass_blk_alloc(fsp,&q,800);
    //printf("fs.sb.free_count = %lld\n", (long long)fs.sb.free_count );
    mass_blk_free(fsp,&q,800);
    //printf("fs.sb.free_count = %lld\n", (long long)fs.sb.free_count );

    reset_q(&q);

    printf("MP Disk block allocation test: DONE\n");
}




// ----------------------------------------------------------------------------
//
// Test inode alloc/free subsystem
//
// ----------------------------------------------------------------------------




void test_mp_inode_alloc( cpfs_fs_t *fs )
{
    errno_t rc;
    cpfs_ino_t i1, i2;

    printf("MP Inode alloc test\n");



    rc = cpfs_alloc_inode( fs, &i1 );
    cpfs_assert( rc == 0 );

    rc = cpfs_alloc_inode( fs, &i2 );
    cpfs_assert( rc == 0 );

    //printf("Inode alloc %lld %lld\n", i1, i2 );


    rc = cpfs_free_inode( fs, i2 );
    cpfs_assert( rc == 0 );

    rc = cpfs_free_inode( fs, i1 );
    cpfs_assert( rc == 0 );


/* can't do double free test in multithread env - we free for second thread accidentally
    rc = cpfs_free_inode( fs, i2 );
    cpfs_assert( rc != 0 );

    rc = cpfs_free_inode( fs, i1 );
    cpfs_assert( rc != 0 );

*/

    printf("MP Inode alloc test: DONE\n");

}




















#endif // __POK_LIBC_STDIO_H__




