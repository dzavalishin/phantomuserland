#ifndef UUFILE_H
#define UUFILE_H

#include <phantom_types.h>
#include <hal.h>
#include <errno.h>
#include <sys/stat.h>

#define FS_MAX_MOUNT 64
#define FS_MAX_MOUNT_PATH 128
#define FS_MAX_PATH_LEN 1024

struct uufile;
struct uufs;

struct uufileops
{
    size_t      (*read)(    struct uufile *f, void *dest, size_t bytes);
    size_t      (*write)(   struct uufile *f, void *dest, size_t bytes);
    errno_t     (*stat)(    struct uufile *f, struct stat *dest);
    int         (*ioctl)(   struct uufile *f, errno_t *err, int request, void *data );

    size_t      (*getpath)( struct uufile *f, void *dest, size_t bytes);

    // returns -1 for non-files?
    size_t      (*getsize)( struct uufile *f);
    errno_t     (*setsize)( struct uufile *f, size_t size);

    errno_t     (*chmod)( struct uufile *f, int mode);
    errno_t     (*chown)( struct uufile *f, int user, int grp);

	// used when clone file
    void *      (*copyimpl)( void *impl);
};


struct uufile
{
    struct uufileops *  ops;
    size_t              pos;
    struct uufs *       fs;
    unsigned            flags;
    const char *	name;   // This entry's name, or zero if none
    void *              impl; // implementation specific

    hal_mutex_t         mutex; // serialize access
};


#define UU_FILE_FLAG_DIR        (1<<0) // Dir
#define UU_FILE_FLAG_NET        (1<<1) // Socket
#define UU_FILE_FLAG_TCP        (1<<2) // Stream
#define UU_FILE_FLAG_UDP        (1<<3) // Dgram

typedef struct uufile uufile_t;


uufile_t *copy_uufile( uufile_t *in );
uufile_t *create_uufile();



struct uufs
{
    char *              name; // Just FS type name
	// called after we got uufile and need really do an open op
    errno_t             (*open)(struct uufile *, int create, int write);
	// Close disposes file - it can't be used after that
    errno_t             (*close)(struct uufile *);

    // Create a file struct for given path
    uufile_t *		(*namei)(const char *filename);

    // Return a file struct for fs root
    uufile_t *		(*root)();
};

typedef struct uufs uufs_t;

struct uutty
{
    struct uufile *     io;
};




#define CHECK_FD_RANGE(_fd) do { if( _fd < 0 || _fd > MAX_UU_FD ) { *err = EBADF; return -1; } } while(0)
#define GETF(_fd) (u->fd[_fd])
#define CHECK_FD_OPEN(__fd)  do { if( GETF(__fd) == 0 ) { *err = EBADF; return -1; } } while(0)

#define CHECK_FD(_cfd) do { CHECK_FD_RANGE(_cfd); CHECK_FD_OPEN(_cfd); } while(0)


extern struct uufileops tcpfs_fops; // Used in sock syscalls
extern struct uufileops udpfs_fops; // Used in sock syscalls

extern struct uufs tcp_fs; // Used in sock syscalls
extern struct uufs udp_fs; // Used in sock syscalls
extern struct uufs dev_fs;
extern struct uufs proc_fs;


#endif // UUFILE_H

