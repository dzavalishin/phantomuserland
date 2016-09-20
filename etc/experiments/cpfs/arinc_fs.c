/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * ARINC-653 File System entry points.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"
#include "arinc_fs.h"



// ----------------------------------------------------------------------------
//
// Main functions
//
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
//
// Volumes support
//
// ----------------------------------------------------------------------------


typedef struct
{
    const char          *name;          // Volume name

    int                 disk_id;        // Disk number (used in driver calls)

    struct cpfs_fs 	*fs;            // FS instance
}
arinc_volume_def_t;


errno_t         arinc_init_filesystems( void );
errno_t         arinc_find_filesystem( const char *full_file_name, struct cpfs_fs **fs; );



