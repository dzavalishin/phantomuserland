#include <unix/uufile.h>
#include <unix/uuprocess.h>

// Unix emulation related things

// Unix syscalls impl

int usys_open(  int *err, uuprocess_t *u, const char *name, int flags, int mode );
int usys_creat( int *err, uuprocess_t *u, const char *name, int mode );
int usys_read(  int *err, uuprocess_t *u, int fd, void *addr, int count );
int usys_write( int *err, uuprocess_t *u, int fd, void *addr, int count );
int usys_close( int *err, uuprocess_t *u, int fd );
int usys_lseek( int *err, uuprocess_t *u, int fd, int offset, int whence );
