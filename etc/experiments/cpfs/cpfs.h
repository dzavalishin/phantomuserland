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

typedef int errno_t;


errno_t 		cpfs_init(void);
errno_t			cpfs_stop(void);


// entry points


errno_t         cpfs_file_open  ( int *file_id, const char *name, int flags, void * user_id_data );
errno_t         cpfs_file_close ( int file_id );


errno_t         cpfs_file_read  ( int file_id, cpfs_size_t pos, const void *data, cpfs_size_t size );
errno_t         cpfs_file_write ( int file_id, cpfs_size_t pos, const void *data, cpfs_size_t size );


#define CPFS_OPEN_FLAG_CREATE  (1<<1)


// access to disk driver

errno_t         cpfs_disk_read( int disk_id, cpfs_blkno_t block, const void *data );
errno_t         cpfs_disk_read( int disk_id, cpfs_blkno_t block, void *data );
errno_t         cpfs_disk_trim( int disk_id, cpfs_blkno_t block ); // Tell SSD we don't need this disk block anymore


// kernel services


void            cpfs_log_error(char *fmt, ... );
void            cpfs_spin_lock();
void            cpfs_spin_unlock();
void            cpfs_mutex_lock();
void            cpfs_mutex_unlock();

