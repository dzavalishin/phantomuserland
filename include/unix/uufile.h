/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * Unix subsystem file machinery.
 *
 *
**/

#ifndef UUFILE_H
#define UUFILE_H

#include <phantom_types.h>
#include <hal.h>
#include <errno.h>
#include <queue.h>
#include <dirent.h>
#include <sys/stat.h>

/**
 * \ingroup Unix
 * @{
**/


#define FS_MAX_MOUNT 64
#define FS_MAX_MOUNT_PATH 128
#define FS_MAX_PATH_LEN 1024

struct uufile;
struct uufs;

struct uufileops
{
    size_t      (*read)(    struct uufile *f, void *dest, size_t bytes);
    size_t      (*write)(   struct uufile *f, const void *src, size_t bytes);

    // Return ENOENT if end of dir reached
    errno_t     (*readdir)( struct uufile *f, struct dirent *dirp );

    // Notify fs driver that curr pos (uufile.pos) is changed, ask to validate
    // If returns nonzero, uufile.pos may be changed again
    errno_t     (*seek)(    struct uufile *f ); 

    errno_t     (*stat)(    struct uufile *f, struct stat *dest);
    int         (*ioctl)(   struct uufile *f, errno_t *err, int request, void *data, int dlen );

    size_t      (*getpath)( struct uufile *f, void *dest, size_t bytes);

    // returns -1 for non-files?
    ssize_t     (*getsize)( struct uufile *f);
    errno_t     (*setsize)( struct uufile *f, size_t size);

    errno_t     (*chmod)( struct uufile *f, int mode);
    errno_t     (*chown)( struct uufile *f, int user, int grp);

    errno_t     (*unlink)( struct uufile *f );

    //! rich man's ioctl - get property
    errno_t	(*getproperty)( struct uufile *f, const char *pName, char *pValue, size_t vlen );
    //! rich man's ioctl - set property
    errno_t	(*setproperty)( struct uufile *f, const char *pName, const char *pValue );
    //! rich man's ioctl - list properties
    errno_t	(*listproperties)( struct uufile *f, int nProperty, char *pValue, size_t vlen );

	// used when clone file
    void *      (*copyimpl)( void *impl);
};


struct uufile
{
    struct uufileops *  ops;
    size_t              pos;
    struct uufs *       fs;
    unsigned            flags;
    const char *        name;   // This entry's name, or zero if none
    void *              impl; // implementation specific

    int                 refcount; // n of refs to this node - TODO!
    hal_mutex_t         mutex; // serialize access
};

typedef struct uufile uufile_t;


#define UU_FILE_FLAG_DIR        (1<<0) // Dir
#define UU_FILE_FLAG_NET        (1<<1) // Socket
#define UU_FILE_FLAG_TCP        (1<<2) // Stream
#define UU_FILE_FLAG_UDP        (1<<3) // Dgram
#define UU_FILE_FLAG_MNT        (1<<4) // Mount point - FS root
#define UU_FILE_FLAG_PIPE       (1<<5) // Pipe
#define UU_FILE_FLAG_RDONLY     (1<<6) // Write op forbidden


//! Is open now, unlink/destroy_uufile will close first
#define UU_FILE_FLAG_OPEN       (1<< 8) 

//! On destroy implementation must be freed
#define UU_FILE_FLAG_FREEIMPL   (1<< 9) 

//! Do not destroy - it's a link to static instance
#define UU_FILE_FLAG_NODESTROY  (1<<10) 



uufile_t *copy_uufile( uufile_t *in );
uufile_t *create_uufile(void);

void set_uufile_name( uufile_t *in, const char *name );


void link_uufile( uufile_t *in );
void unlink_uufile( uufile_t *in );

// use unlink
//void destroy_uufile(uufile_t *f);




typedef struct dir_ent
{
    queue_chain_t       chain;

    const char *        name; // malloc'ed
    int                 flags;
    uufile_t *          uufile;
    void *              unused;
} dir_ent_t;


uufile_t *create_dir(void);

//! Unlink by name
errno_t unlink_dir_name( uufile_t *dir, const char *name );

//! Unlink by contaned uufile
errno_t unlink_dir_ent( uufile_t *dir, uufile_t *deu );

//! Find or create dir ent
uufile_t *lookup_dir( uufile_t *dir, const char *name, int create, uufile_t *(*createf)() );

//! Return n-th entry's name.
errno_t get_dir_entry_name( uufile_t *dir, int n, char *name );

//! General impl of read syscall for dir
int common_dir_read(struct uufile *f, void *dest, size_t bytes);

extern struct uufileops common_dir_fops;



struct uufs
{
    char *              name; // Just FS type name

    void *              impl; // This instance's implementation state

	//! called after we got uufile and need really do an open op
    errno_t             (*open)(struct uufile *, int create, int write);

	//! Close disposes file - it can't be used after that
    errno_t             (*close)(struct uufile *);

    //! Create a file struct for given path
    uufile_t *          (*namei)(struct uufs *fs, const char *filename);

    errno_t             (*symlink)(struct uufs *fs, const char *src, const char *dst);
    errno_t             (*mkdir)(struct uufs *fs, const char *path);


    //! Return a file struct for fs root
    uufile_t *          (*root)(struct uufs *fs);

	//! Dispose fs itself
    errno_t             (*dismiss)(struct uufs *fs);
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


//! Make absolute pathname having prefix and possible suffix
errno_t uu_make_absname( char *out_path, const char *base, const char *add );

//! Break path into the pieces
int uu_break_path( const char *in, int maxpart, const char *oname[], size_t olen[] );

//! True if fn is absolute
int uu_is_absname( const char *fn );

struct uuprocess;

//! Makes sure that buf contains correct abs fn for given
//! (possibly relative) filaname and process (cwd)
errno_t uu_normalize_path( char *buf, const char *filename, struct uuprocess *p );

#define AUTO_MOUNT_FLAG_AUTORUN (1<<0)
//#define AUTO_MOUNT_FLAG_AUTORUN (1<<0)

errno_t auto_mount( const char *name, uufs_t *fs, char *out_mount_point, size_t out_mount_point_buf_size, int flags );



#endif // UUFILE_H

