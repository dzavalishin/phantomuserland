/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Interface definition.
 *
 *
**/

#ifndef CPFS_H
#define CPFS_H


#include "cpfs_types.h"
#include "cpfs_defs.h"

#define cpfs_assert(__check)

struct cpfs_fs;

errno_t 	cpfs_init( struct cpfs_fs *fs );
errno_t		cpfs_stop( struct cpfs_fs *fs );

errno_t 	cpfs_mount ( struct cpfs_fs *fs );
errno_t		cpfs_umount( struct cpfs_fs *fs );

errno_t 	cpfs_mkfs( struct cpfs_fs *fs, cpfs_blkno_t disk_size );
errno_t 	cpfs_fsck( struct cpfs_fs *fs, int fix );

// entry points


errno_t         cpfs_file_open  ( struct cpfs_fs *fs, int *file_id, const char *name, int flags, void * user_id_data );
errno_t         cpfs_file_close ( int file_id );


errno_t         cpfs_file_read  ( int file_id, cpfs_size_t pos, void *data, cpfs_size_t size );
errno_t         cpfs_file_write ( int file_id, cpfs_size_t pos, const void *data, cpfs_size_t size );


#define CPFS_OPEN_FLAG_CREATE  (1<<1)


// access to disk driver

errno_t         cpfs_disk_read( int disk_id, cpfs_blkno_t block, void *data );
errno_t         cpfs_disk_write( int disk_id, cpfs_blkno_t block, const void *data );

errno_t         cpfs_disk_trim( int disk_id, cpfs_blkno_t block ); // Tell SSD we don't need this disk block anymore - TODO use me


// kernel services


void            cpfs_log_error( const char *fmt, ... );

void 		cpfs_panic( const char *fmt, ... );

/*
typedef void * cpfs_spinlock;

void            cpfs_spin_lock(cpfs_spinlock l);
void            cpfs_spin_unlock(cpfs_spinlock l);
*/

typedef void * cpfs_mutex;
typedef void * cpfs_mutex_t;
void            cpfs_mutex_init( cpfs_mutex * );

void            cpfs_mutex_lock( cpfs_mutex m );
void            cpfs_mutex_unlock( cpfs_mutex m );

cpfs_time_t	cpfs_get_current_time(void);


#endif // CPFS_H
