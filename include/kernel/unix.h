#include <unix/uufile.h>
#include <unix/uuprocess.h>

// Unix emulation related things

uufile_t *uu_namei(const char *filename);


// Unix syscalls impl

int usys_open(  int *err, uuprocess_t *u, const char *name, int flags, int mode );
int usys_creat( int *err, uuprocess_t *u, const char *name, int mode );
int usys_read(  int *err, uuprocess_t *u, int fd, void *addr, int count );
int usys_write( int *err, uuprocess_t *u, int fd, void *addr, int count );
int usys_close( int *err, uuprocess_t *u, int fd );
int usys_lseek( int *err, uuprocess_t *u, int fd, int offset, int whence );
int usys_ioctl( int *err, uuprocess_t *u, int fd, int request, void *data );


int usys_chdir( int *err, uuprocess_t *u, const char *path );
int usys_fchdir(int *err, uuprocess_t *u, int fd );

int usys_mount( int *err, uuprocess_t *u, const char *source, const char *target, const char *fstype, int flags, const void *data );
int usys_umount(int *err, uuprocess_t *u, const char *target, int flags );

int usys_stat(  int *err, uuprocess_t *u, const char *path, struct stat *data, int statlink );
int usys_fstat( int *err, uuprocess_t *u, int fd, struct stat *data, int statlink );

int usys_truncate( int *err, uuprocess_t *u, const char *path, off_t length);
int usys_ftruncate(int *err, uuprocess_t *u, int fd, off_t length);

int usys_fchmod( int *err, uuprocess_t *u, int fd, int mode );
int usys_fchown( int *err, uuprocess_t *u, int fd, int user, int grp );



int usys_gethostname( int *err, uuprocess_t *u, char *data, size_t len );
int usys_sethostname( int *err, uuprocess_t *u, const char *data, size_t len );

