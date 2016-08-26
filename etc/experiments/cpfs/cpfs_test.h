/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Test units definitions.
 *
 *
**/

#ifndef CPFS_TEST_H
#define CPFS_TEST_H

#include "cpfs_local.h"

//extern cpfs_fs_t fs;



void    test_superblock(cpfs_fs_t *fsp);

void	test_disk_alloc(cpfs_fs_t *fsp);

void    test_inode_blkmap(cpfs_fs_t *fsp); 	// test file block allocation with inode

void    test_inode_io(cpfs_fs_t *fsp); // read/write directly with inode, no file name

void    test_inode_alloc(cpfs_fs_t *fsp);

void    test_path(cpfs_fs_t *fsp);                // Parse/descend path name

void    test_directory(cpfs_fs_t *fsp);        // Create/lookup/destroy directory entries
void    test_file_create(cpfs_fs_t *fsp); 	// create, open and destroy multiple files, try open deleted files
void    test_file_data(cpfs_fs_t *fsp);        // Create, write, close, reopen, read and compare data, in a mixed way
void    test_mutithreaded(cpfs_fs_t *fsp);     // Do mix of prev tests in 10 threads, starting tests in random order



#define test_str_eq( __s1, __s2 ) if( strcmp( (__s1), (__s2) ) ) cpfs_panic( "test failed " __FILE__ " @ %d: %s != %s\n", __LINE__, __s1, __s2 )
#define test_int_eq( __s1, __s2 ) if( (__s1) != (__s2) ) cpfs_panic( "test failed " __FILE__ " @ %d: %d != %d\n", __LINE__, __s1, __s2 )



void cpfs_debug_fdump( const char *fn, void *p, unsigned size ); // dump some data to file

#endif // CPFS_TEST_H
