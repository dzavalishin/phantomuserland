/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel functions implementing Unix syscalls.
 * Unix emulation related things.
 *
**/

#include <unix/uufile.h>
#include <unix/uuprocess.h>
#include <sys/socket.h>
#include <dirent.h>
#include <signal.h>

/**
 * \ingroup Unix
 * @{
**/

struct sockaddr;


// Find uufile_t object for path, applying cwd if possible
uufile_t *uu_namei(const char *filename, uuprocess_t *u );

// find fs by path. rest is FS_MAX_PATH_LEN bytes
uufs_t * uu_findfs(const char *filename, char *rest );


// Allocate a new fd in u for f
int uu_find_fd( uuprocess_t *u, uufile_t *f  );


// Unix syscalls impl

int usys_open(  int *err, uuprocess_t *u, const char *name, int flags, int mode );
int usys_creat( int *err, uuprocess_t *u, const char *name, int mode );
int usys_read(  int *err, uuprocess_t *u, int fd, void *addr, int count );
int usys_write( int *err, uuprocess_t *u, int fd, const void *addr, int count );
int usys_close( int *err, uuprocess_t *u, int fd );
int usys_lseek( int *err, uuprocess_t *u, int fd, int offset, int whence );
int usys_ioctl( int *err, uuprocess_t *u, int fd, int request, void *data, size_t len );

int usys_pipe(int *err, uuprocess_t *u, int *fds );

int usys_dup2(int *err, uuprocess_t *u, int src_fd, int dst_fd );
int usys_dup(int *err, uuprocess_t *u, int src_fd );

int usys_symlink(int *err, uuprocess_t *u, const char *src, const char *dst );
int usys_mkdir( int *err, uuprocess_t *u, const char *path );

int usys_fcntl( int *err, uuprocess_t *u, int fd, int cmd, int arg );


int usys_rm( int *err, uuprocess_t *u, const char *name );


int usys_chdir( int *err, uuprocess_t *u, const char *path );
int usys_fchdir(int *err, uuprocess_t *u, int fd );

int usys_getcwd( int *err, uuprocess_t *u, char *buf, int bufsize );


int usys_mount( int *err, uuprocess_t *u, const char *source, const char *target, const char *fstype, int flags, const void *data );
int usys_umount(int *err, uuprocess_t *u, const char *target, int flags );

int usys_stat(  int *err, uuprocess_t *u, const char *path, struct stat *data, int statlink );
int usys_fstat( int *err, uuprocess_t *u, int fd, struct stat *data, int statlink );

int usys_truncate( int *err, uuprocess_t *u, const char *path, off_t length);
int usys_ftruncate(int *err, uuprocess_t *u, int fd, off_t length);

int usys_fchmod( int *err, uuprocess_t *u, int fd, int mode );
int usys_fchown( int *err, uuprocess_t *u, int fd, int user, int grp );

int usys_readdir(int *err, uuprocess_t *u, int fd, struct dirent *dirp );

errno_t usys_setproperty( int *err, uuprocess_t *u, int fd, const char *pName, const char *pValue );
errno_t usys_getproperty( int *err, uuprocess_t *u, int fd, const char *pName, char *pValue, size_t vlen );
errno_t usys_listproperties( int *err, uuprocess_t *u, int fd, int nProperty, char *buf, int buflen );





int usys_gethostname( int *err, uuprocess_t *u, char *data, size_t len );
int usys_sethostname( int *err, uuprocess_t *u, const char *data, size_t len );

int usys_getsockname(int *err, uuprocess_t *u, int fd, struct sockaddr *name, socklen_t *namelen);
int usys_getpeername(int *err, uuprocess_t *u, int fd, struct sockaddr *name, socklen_t *namelen);


int usys_getsockopt(int *err, uuprocess_t *u, int fd, int level, int optname, void *optval, socklen_t *optlen);
int usys_setsockopt(int *err, uuprocess_t *u, int fd, int level, int optname, const void *optval, socklen_t optlen);


int usys_socket(int *err, uuprocess_t *u, int domain, int type, int protocol);
int usys_bind(  int *err, uuprocess_t *u, int fd, const struct sockaddr *my_addr, socklen_t addrlen);
int usys_listen(int *err, uuprocess_t *u, int fd, int backlog);
int usys_accept(int *err, uuprocess_t *u, int fd, struct sockaddr *acc_addr, socklen_t *addrlen);
int usys_connect(int *err, uuprocess_t *u, int fd, const struct sockaddr *ia, socklen_t addrlen);

int usys_socketpair( int *err, uuprocess_t *u, int domain, int type, int protocol, int socket_vector[2]);



ssize_t usys_recv(int *err, uuprocess_t *u, int fd, void *buf, size_t len, int flags);
ssize_t usys_recvfrom(int *err, uuprocess_t *u, int fd, void *buf, size_t len, int flags,
                      struct sockaddr *from, socklen_t *fromlen);
ssize_t usys_recvmsg(int *err, uuprocess_t *u, int fd, struct msghdr *msg, int flags);

ssize_t usys_send(int *err, uuprocess_t *u, int fd, const void *buf, size_t len, int flags);
ssize_t usys_sendto(int *err, uuprocess_t *u, int fd, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
ssize_t usys_sendmsg(int *err, uuprocess_t *u, int fd, const struct msghdr *msg, int flags);




int usys_kill(int *err, uuprocess_t *u, int pid, int sig);

int usys_waitpid(int *err, uuprocess_t *u, int pid, int *status, int options);

int usys_run( int *err, uuprocess_t *u,  const char *fname, const char **uav, const char **uep, int flags );

int usys_brk( errno_t *err, uuprocess_t *u, int ds_size );




int usys_sigpending( int *err, uuprocess_t *u, sigset_t * set);
sighandler_t usys_signal( int *err, uuprocess_t *u, int signum, sighandler_t handler);



void usys_phantom_runclass( errno_t *err, uuprocess_t *u, const char *cname, int nmethod );



