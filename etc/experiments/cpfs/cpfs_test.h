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


void    test_superblock(void);

void	test_disk_alloc(void);

void    test_inode_blkmap(void); 	// test file block allocation with inode

void    test_inode_io(void); // read/write directly with inode, no file name

void    test_inode_alloc(void);


void    test_directory(void);        // Create/lookup/destroy directory entries
void    test_file_create(void); 	// create, open and destroy multiple files, try open deleted files
void    test_file_data(void);        // Create, write, close, reopen, read and compare data, in a mixed way
void    test_mutithreaded(void);     // Do mix of prev tests in 10 threads, starting tests in random order

