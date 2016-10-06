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




static void *diskmem;
static int   diskmem_sz = 10*1000L*4096;


#warning init me


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

static void real_main(void);


void main(void)
{
    real_main();
    STOP_SELF();
}  

static void first_process(void)
{
#warning inin from here?
}

// ----------------------------------------------------------------------------
//
// Main
//
// ----------------------------------------------------------------------------


static void
real_main(void)
{
    diskmem = calloc ( 1, diskmem_sz );
    cpfs_assert( diskmem != 0 );
}


