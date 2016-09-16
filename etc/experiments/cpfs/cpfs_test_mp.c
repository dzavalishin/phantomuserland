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
    if( rc )  cpfs_panic( "create 1 %d", rc );

    rc = cpfs_file_open( fs, &fd2, fn2, O_CREAT, 0 );
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


    printf("MP File API data test: DONE\n");


}






