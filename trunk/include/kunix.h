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

void kernel_unix_init(void);


errno_t k_open( int *fd, const char *name, int flags, int mode );
errno_t k_read( int *nread, int fd, void *addr, int count );
errno_t k_write( int *nread, int fd, const void *addr, int count );
errno_t k_seek( int *pos, int fd, int offset, int whence );
errno_t k_stat( const char *path, struct stat *data, int statlink );
errno_t k_close( int fd );

// Data buffer is malloc'ed and to be freed by caller
errno_t k_load_file( void **odata, int *osize, const char *fname );



#endif // KUNIX_H
