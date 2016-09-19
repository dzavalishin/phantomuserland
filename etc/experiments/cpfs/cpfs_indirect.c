/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Indirect file blocks support.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"



// CPFS_INDIRECT_PER_BLK - how many block pointers are in indirect block

/**
 *
 * indexes      - return array of indexes into indirect blocks, 
 * start_index  - start position in indexes which is used, end position is allways the end of array
 *
 * logical      - input logical block address (0 to last file blk no)
 *
 * Returns:
 *		0 on success, E2BIG if logical is too big for this file system structure.
 *
**/

errno_t calc_indirect_positions( cpfs_fs_t *fs, cpfs_blkno_t indexes[CPFS_MAX_INDIR], int *start_index, cpfs_blkno_t ilogical )
{
    (void) fs;

    int ai;
    cpfs_blkno_t logical = ilogical;

    if( logical < CPFS_INO_DIR_BLOCKS )
    {
        *start_index = CPFS_MAX_INDIR; // no array slot was used
        return 0;
    }


    logical -= CPFS_INO_DIR_BLOCKS;

    // 1 level of indirection

    if( logical < CPFS_INDIRECT_PER_BLK )
    {
        *start_index = CPFS_MAX_INDIR-1;
        goto fill_array;
    }

    logical -= CPFS_INDIRECT_PER_BLK;

    // 2 levels of indirection

    if( logical < 1LL*CPFS_INDIRECT_PER_BLK*CPFS_INDIRECT_PER_BLK )
    {
        *start_index = CPFS_MAX_INDIR-2;
        goto fill_array;
    }

    logical -= CPFS_INDIRECT_PER_BLK*CPFS_INDIRECT_PER_BLK;

    // 3 levels of indirection

    if( logical < 1LL*CPFS_INDIRECT_PER_BLK*CPFS_INDIRECT_PER_BLK*CPFS_INDIRECT_PER_BLK )
    {
        *start_index = CPFS_MAX_INDIR-3;
        goto fill_array;
    }

    logical -= CPFS_INDIRECT_PER_BLK*CPFS_INDIRECT_PER_BLK*CPFS_INDIRECT_PER_BLK;

    // 4 levels of indirection

    if( logical < 1LL*CPFS_INDIRECT_PER_BLK*CPFS_INDIRECT_PER_BLK*CPFS_INDIRECT_PER_BLK*CPFS_INDIRECT_PER_BLK )
    {
        *start_index = CPFS_MAX_INDIR-4;
        goto fill_array;
    }

    logical -= CPFS_INDIRECT_PER_BLK*CPFS_INDIRECT_PER_BLK*CPFS_INDIRECT_PER_BLK*CPFS_INDIRECT_PER_BLK;

    return E2BIG;

fill_array:
    ai = CPFS_MAX_INDIR-1; // array index, start from right
    int cnt = CPFS_MAX_INDIR - *start_index;

    while( cnt > 0 ) //logical > 0 )
    {
        if( ai < 0 ) return E2BIG; // can't happen

        int pos = logical % CPFS_INDIRECT_PER_BLK;
        //logical -= pos;
        logical /= CPFS_INDIRECT_PER_BLK;

        indexes[ai] = pos;
        ai--;
        cnt--;

        cpfs_assert( ai >= 0 );
    }

    // ai decrements one more time after filling last (lowest) slot, so we compare to ai+1
    if((ai+1) != *start_index)
    {
        printf("calc_indir( %lld ) -> ai=%d, start_index=%d, indexes = %lld, %lld, %lld, %lld\n", (long long)ilogical, ai, *start_index, (long long)indexes[0], (long long)indexes[1], (long long)indexes[2], (long long)indexes[3] );
    }
    cpfs_assert( (ai+1) == *start_index );

    return 0;
}









// ----------------------------------------------------------------------------
//
// Free indirect blocks to given depth
//
// ----------------------------------------------------------------------------





errno_t
cpfs_free_indirect( cpfs_fs_t *fs, cpfs_blkno_t indir_blk, int depth )
{
    //errno_t rc, rc1;
    unsigned int i;

    if( indir_blk == 0 ) return 0;

    struct cpfs_indir *iblk = cpfs_lock_blk( fs, indir_blk );
    cpfs_assert( iblk );

    // TODO check magic

    for( i = 0; i < CPFS_INDIRECT_PER_BLK; i++ )
    {
        cpfs_blkno_t child = iblk->child[i];

        if( child == 0 ) continue;

        if( depth == 1 )
            cpfs_free_disk_block( fs, child );
        else
            cpfs_free_indirect( fs, child, depth-1 );
    }

    cpfs_unlock_blk( fs, indir_blk );
    cpfs_free_disk_block( fs, indir_blk );
    return 0;
}







