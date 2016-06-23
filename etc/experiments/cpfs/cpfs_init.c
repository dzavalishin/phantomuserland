/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Init/shutdown.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"


errno_t
cpfs_init(void)
{
    errno_t rc;

    // TODO assert( sizeof(struct cpfs_dir_entry) < CPFS_DIR_REC_SIZE )

    rc = cpfs_init_sb();
    if( rc ) return rc;


    fic_refill(); // fill list of free inodes
    return 0;
}


errno_t cpfs_stop(void)
{

    // TODO check locked rcords
    // TODO flush?
    return 0;
}

