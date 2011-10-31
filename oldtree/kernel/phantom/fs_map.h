#ifndef FS_MAP_H
#define FS_MAP_H


#include <errno.h>
#include <disk.h>

//! Scan through all FS probe funcs, select one (or zero), returns 0 if FS is found
errno_t lookup_fs(phantom_disk_partition_t *p);


errno_t fs_probe_phantom(phantom_disk_partition_t *p);
errno_t fs_use_phantom(phantom_disk_partition_t *p);

#if HAVE_UNIX

errno_t fs_probe_fat(phantom_disk_partition_t *p);
errno_t fs_start_fat( phantom_disk_partition_t *p );

errno_t fs_probe_ext2(phantom_disk_partition_t *p);
errno_t fs_start_ext2(phantom_disk_partition_t *p);

errno_t fs_probe_cd(phantom_disk_partition_t *p);
errno_t fs_start_cd(phantom_disk_partition_t *p);

#endif // HAVE_UNIX



#endif // FS_MAP_H


