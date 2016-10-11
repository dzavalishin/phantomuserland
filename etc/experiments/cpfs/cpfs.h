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

#define cpfs_assert(__check) if(!(__check)) cpfs_panic("Assert failed in " __FILE__ " @ %d func %s\n\t" #__check "\n\n", __LINE__, __func__ );

extern int TRACE;
extern int TRACE_TAB;

// TODO remove from here, can't be compiled in JetOS, compile with #ifdef
#ifndef __POK_LIBC_STDIO_H__
extern FILE *fsck_scan_dir_log_file;
extern FILE *fsck_scan_ino_log_file;
#endif

#ifndef O_CREAT
#define	O_CREAT		0x0200	/* open with file create */

#define	O_RDONLY	0		/* +1 == FREAD */
#define	O_WRONLY	1		/* +1 == FWRITE */
#define	O_RDWR		2		/* +1 == FREAD|FWRITE */

#endif

struct cpfs_fs;

errno_t 	cpfs_init( struct cpfs_fs *fs );
errno_t		cpfs_stop( struct cpfs_fs *fs );

errno_t 	cpfs_mount ( struct cpfs_fs *fs );
errno_t		cpfs_umount( struct cpfs_fs *fs );

errno_t 	cpfs_mkfs( struct cpfs_fs *fs, cpfs_blkno_t disk_size );
errno_t 	cpfs_fsck( struct cpfs_fs *fs, int fix );

errno_t 	cpfs_fs_stat( struct cpfs_fs *fs, cpfs_blkno_t *disk_size, cpfs_blkno_t *disk_free );
errno_t 	cpfs_fs_dump( struct cpfs_fs *fs );

// entry points


errno_t         cpfs_file_open  ( struct cpfs_fs *fs, int *file_id, const char *name, int flags, void * user_id_data );
errno_t         cpfs_file_close ( int file_id );


errno_t         cpfs_file_read  ( int file_id, cpfs_size_t pos, void *data, cpfs_size_t size );
errno_t         cpfs_file_write ( int file_id, cpfs_size_t pos, const void *data, cpfs_size_t size );


errno_t         cpfs_mkdir( struct cpfs_fs *fs, const char *path, void * user_id_data );
errno_t 	cpfs_file_unlink( struct cpfs_fs *fs, const char *name, void * user_id_data );
errno_t 	cpfs_file_stat( struct cpfs_fs *fs, const char *name, void * user_id_data, struct cpfs_stat *stat );
errno_t 	cpfs_fd_stat( int file_id, struct cpfs_stat *stat );

#define CPFS_OPEN_FLAG_CREATE  (1<<1)


// access to disk driver

errno_t         cpfs_disk_read( int disk_id, cpfs_blkno_t block, void *data );
errno_t         cpfs_disk_write( int disk_id, cpfs_blkno_t block, const void *data );

errno_t         cpfs_disk_trim( int disk_id, cpfs_blkno_t block ); // Tell SSD we don't need this disk block anymore


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
void            cpfs_mutex_init( cpfs_mutex *mp );

void            cpfs_mutex_lock( cpfs_mutex m );
void            cpfs_mutex_unlock( cpfs_mutex m );

void            cpfs_mutex_stop( cpfs_mutex m );


cpfs_time_t	cpfs_get_current_time(void);

errno_t 	cpfs_os_run_idle_thread( void* (*func_p)(void *arg), void *arg ); // Request OS to start thread


// called by fs to find out if user can do something

typedef enum { cpfs_r_other, cpfs_r_read, cpfs_r_write, cpfs_r_stat, cpfs_r_mkdir, cpfs_r_unlink } cpfs_right_t;

errno_t         cpfs_os_access_rights_check( struct cpfs_fs *fs, cpfs_right_t t, void *user_id_data, const char *fname );

#endif // CPFS_H

