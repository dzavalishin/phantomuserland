#if HAVE_UNIX

#include <unix/uufile.h>
#include <unix/uuprocess.h>
#include <kernel/unix.h>

//static uufs_t *rootfs;
extern struct uufs root_fs;

// Creates uufile for given path name
uufile_t *uu_namei(const char *filename)
{
    if( filename[0] == '/' && filename[1] == 0 )
        return copy_uufile( root_fs.root(&root_fs) );

    return root_fs.namei(&root_fs, filename);
}

#endif // HAVE_UNIX

