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

	// used when clone file
	void *		(*copyimpl)( void *impl); 
};


struct uufile
{
    struct uufileops *  ops;
    size_t              pos;
    struct uufs *       fs;
    void *              impl; // implementation specific

    hal_mutex_t         mutex; // serialize access
};


typedef struct uufile uufile_t;


uufile_t *copy_uufile( uufile_t *in );
uufile_t *create_uufile();



struct uufs
{
	//char			name[FS_MAX_MOUNT_PATH];
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


#endif // UUFILE_H

