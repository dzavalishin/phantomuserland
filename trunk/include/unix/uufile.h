#ifndef UUFILE_H
#define UUFILE_H

#include <phantom_types.h>
#include <hal.h>
#include <errno.h>

struct uufile;
struct uufs;

struct uufileops
{
    size_t      (*read)(    struct uufile *f, void *dest, size_t bytes);
    size_t      (*write)(   struct uufile *f, void *dest, size_t bytes);
    //errno_t     (*stat)(    struct uufile *f, struct ??);
    //errno_t     (*ioctl)(   struct uufile *f, struct ??);

    size_t      (*getpath)( struct uufile *f, void *dest, size_t bytes);

    // returns -1 for non-files?
    size_t      (*getsize)( struct uufile *f);
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
    struct uufile *     (*open)(const char *name, int create, int write);
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

