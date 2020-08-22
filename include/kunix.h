/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * In-kernel stdio-like stuff.
 *
 *
**/

#ifndef KUNIX_H
#define KUNIX_H

#include <errno.h>
#include <sys/stat.h>

void kernel_unix_init(void);

/**
 *
 * When Phantom kernel code is running in user mode (pvm_test)
 * we are not sure if struct stat is the same in our headers and
 * in host's GCC lib. Hence k_sat can't be used. 
 *
 * Hence k_stat_size is a palliative solution.
 *
**/

errno_t k_open( int *fd, const char *name, int flags, int mode );
errno_t k_read( int *nread, int fd, void *addr, int count );
errno_t k_write( int *nwritten, int fd, const void *addr, int count );
errno_t k_seek( int *pos, int fd, int offset, int whence );
errno_t k_stat( const char *path, struct stat *data, int statlink );
errno_t k_stat_size( const char *path, unsigned int *size ); // just get file size
errno_t k_close( int fd );

// Data buffer is malloc'ed and to be freed by caller
errno_t k_load_file( void **odata, size_t *osize, const char *fname );



#endif // KUNIX_H
