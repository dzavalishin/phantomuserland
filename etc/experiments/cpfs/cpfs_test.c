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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


void    test_superblock(void)
{
}


#define QSZ (2048*100)

cpfs_blkno_t    tda_q[QSZ];
int             tda_q_pp = 0;
int             tda_q_gp = 0;

static void reset_q(void)
{
    tda_q_pp = 0;
    tda_q_gp = 0;
}


static void mass_blk_alloc(int cnt)
{
    while(cnt-->0)
    {
        cpfs_blkno_t blk = cpfs_alloc_disk_block();
        if( !blk ) cpfs_panic("can't allocate block");
        if(tda_q_pp >= QSZ) cpfs_panic("test out of q");
        tda_q[tda_q_pp++] = blk;
    }
}

static void mass_blk_free(int cnt)
{
    while(cnt-->0)
    {
        cpfs_blkno_t blk = tda_q[tda_q_gp++];
        if( !blk ) cpfs_panic("mass_blk_free blk 0");
        if(tda_q_gp >= QSZ) cpfs_panic("mass_blk_free test out of q");
        cpfs_free_disk_block( blk );
    }
}


void	test_disk_alloc(void)
{
    printf("Disk block allocation test: mixed alloc/free\n");
    //printf("fs_sb.free_count = %lld\n", (long long)fs_sb.free_count );

    cpfs_blkno_t initial_free = fs_sb.free_count;

    mass_blk_alloc(1);   // +
    mass_blk_alloc(120); // +
    mass_blk_free(34);   // -
    mass_blk_alloc(40);  // +
    mass_blk_free(120);  // -
    mass_blk_alloc(80);  // +
    mass_blk_free(40);   // -
    mass_blk_alloc(34);  // +
    mass_blk_free(80);   // -
    mass_blk_free(1);    // -

    if( initial_free != fs_sb.free_count )
    {
        printf("FAIL: initial_free (%lld) != fs_sb.free_count (%lld)\n", (long long)initial_free, (long long)fs_sb.free_count );
    }

    // Now do max possible run twice

    printf("Disk block allocation test: big runs\n");
    //printf("fs_sb.free_count = %lld\n", (long long)fs_sb.free_count );
    reset_q();
    mass_blk_alloc(1700);
    //printf("fs_sb.free_count = %lld\n", (long long)fs_sb.free_count );
    mass_blk_free(1700);
    //printf("fs_sb.free_count = %lld\n", (long long)fs_sb.free_count );
    reset_q();
    mass_blk_alloc(1700);
    //printf("fs_sb.free_count = %lld\n", (long long)fs_sb.free_count );
    mass_blk_free(1700);
    //printf("fs_sb.free_count = %lld\n", (long long)fs_sb.free_count );

    reset_q();

    // TODO attempt to allocate over the end of disk, check for graceful deny

    printf("Disk block allocation test: DONE\n");
}



