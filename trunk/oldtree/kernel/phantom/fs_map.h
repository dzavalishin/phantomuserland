#ifndef FS_MAP_H
#define FS_MAP_H


#include <errno.h>
#include <disk.h>

//! Scan through all FS probe funcs, select one (or zero), returns 0 if FS is found
errno_t lookup_fs(phantom_disk_partition_t *p);


errno_t fs_probe_phantom(phantom_disk_partition_t *p);
errno_t fs_probe_fat(phantom_disk_partition_t *p);
errno_t fs_probe_ext2(phantom_disk_partition_t *p);
errno_t fs_probe_cd(phantom_disk_partition_t *p);


errno_t fs_start_tiny_fat( phantom_disk_partition_t * );


#endif // FS_MAP_H


