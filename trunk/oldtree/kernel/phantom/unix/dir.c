/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Unix dir code.
 *
 *
**/

#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "dir"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <unix/uufile.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <queue.h>
#include <hal.h>
#include <dirent.h>

static ssize_t     dir_getsize( struct uufile *f);
static size_t      dir_getpath( struct uufile *f, void *dest, size_t bytes);

static size_t      dir_write(   struct uufile *f, const void *src, size_t bytes);
static size_t      dir_read(    struct uufile *f, void *dest, size_t bytes);

static errno_t     dir_stat( struct uufile *f, struct stat *dest );

static void *      dir_copyimpl( void *impl );


struct uufileops common_dir_fops =
{
    .read 	= dir_read,
    .write 	= dir_write,

    .getpath 	= dir_getpath,
    .getsize 	= dir_getsize,

    .copyimpl   = dir_copyimpl,

    .stat       = dir_stat,
    //.ioctl      = dir_ioctl,
};




typedef struct dir_impl
{
    int                 refcount;
    hal_mutex_t         mutex;
    int                 nentries;
    queue_head_t        list; // TODO! SLOOOOW!
} dir_t;


static void create_dir_impl( uufile_t *dir )
{
    assert(dir->flags & UU_FILE_FLAG_DIR);

    if( dir->impl == 0 )
    {
        dir->impl = calloc( 1, sizeof(struct dir_impl) );

        assert(dir->impl);

        struct dir_impl *i = dir->impl;

        queue_init( &(i->list) );
        hal_mutex_init( &(i->mutex), "UnixDir" );
        i->refcount = 0;
    }

    dir->ops = &common_dir_fops;
}



uufile_t *create_dir()
{
    uufile_t *dir = create_uufile();

    dir->flags = UU_FILE_FLAG_DIR;

    create_dir_impl(dir);

    return dir;
}




uufile_t *lookup_dir( uufile_t *dir, const char *name, int create, uufile_t *(*createf)() )
{
    create_dir_impl(dir);

    struct dir_impl *i = dir->impl;

    hal_mutex_lock(&(i->mutex));

    dir_ent_t *de;
    queue_iterate( &(i->list), de, dir_ent_t *, chain )
    {
        SHOW_FLOW( 10, "scan dir '%s' for '%s'", de->name, name );
        if( 0 == strcmp( de->name, name ) )
            goto ret;
    }

    if(!create)
    {
        hal_mutex_unlock(&(i->mutex));
        return 0;
    }

    SHOW_FLOW( 7, "dir %p add '%s'", dir, name );

    uufile_t *deu = createf();

    de = calloc( 1, sizeof(dir_ent_t) );
    assert(de);
    de->name = strdup( name );
    de->uufile = deu;
    queue_enter( &(i->list), de, dir_ent_t *, chain );
    link_uufile( de->uufile );
    i->nentries--;
    set_uufile_name( de->uufile, name );

    ret:
    hal_mutex_unlock(&(i->mutex));
    return de->uufile;
}


errno_t unlink_dir_name( uufile_t *dir, const char *name )
{
    if(! (dir->flags & UU_FILE_FLAG_DIR) )
        return ENOTDIR;

    struct dir_impl *i = dir->impl;
    if( i == 0 )
        return ENOENT;

    hal_mutex_lock(&(i->mutex));

    int err = 0;

    dir_ent_t *de;
    queue_iterate( &(i->list), de, dir_ent_t *, chain )
    {
        if( 0 == strcmp( de->name, name ) )
        {
            queue_remove( &(i->list), de, dir_ent_t *, chain );
            unlink_uufile( de->uufile );
            i->nentries--;
            goto ret;
        }
    }
    err = ENOENT;

    ret:
    hal_mutex_unlock(&(i->mutex));
    return err;
}


errno_t unlink_dir_ent( uufile_t *dir, uufile_t *deu )
{
    if(! (dir->flags & UU_FILE_FLAG_DIR) )
        return ENOTDIR;

    struct dir_impl *i = dir->impl;
    if( i == 0 )
        return ENOENT;

    hal_mutex_lock(&(i->mutex));

    int err = 0;

    dir_ent_t *de;
    queue_iterate( &(i->list), de, dir_ent_t *, chain )
    {
        if( de->uufile == deu )
        {
            queue_remove( &(i->list), de, dir_ent_t *, chain );
            unlink_uufile( de->uufile );
            i->nentries--;
            goto ret;
        }
    }
    err = ENOENT;

    ret:
    hal_mutex_unlock(&(i->mutex));
    return err;
}


errno_t get_dir_entry_name( uufile_t *dir, int pos, char *name )
{
    create_dir_impl(dir);

    struct dir_impl *i = dir->impl;
    hal_mutex_lock(&(i->mutex));

    SHOW_FLOW( 7, "scan dir %p for pos %d", dir, pos );

    int ne = 0;
    dir_ent_t *de;
    queue_iterate( &(i->list), de, dir_ent_t *, chain )
    {
        SHOW_FLOW( 10, "scan dir pos %d = '%s'", ne, de->name );

        if( ne++ != pos )
            continue;

        strlcpy( name, de->name, FS_MAX_PATH_LEN );
        hal_mutex_unlock(&(i->mutex));
        return 0;
    }

    hal_mutex_unlock(&(i->mutex));
    return ENOENT;
}

// -----------------------------------------------------------------------
// Methods
// -----------------------------------------------------------------------



// TODO use pool for impl!
static void *      dir_copyimpl( void *impl )
{
    struct dir_impl *i = impl;
    i->refcount++;
    //(void) impl;
    return impl; //strdup(impl);
}


static errno_t     dir_stat( struct uufile *f, struct stat *dest )
{
    //const char *name = f->name;

    SHOW_FLOW( 7, "stat %s", f->name );

    memset( dest, 0, sizeof(struct stat) );

    dest->st_nlink = 1;
    dest->st_uid = -1;
    dest->st_gid = -1;

    dest->st_size = 0;
    dest->st_mode = 0777; // rwxrwxrwx

    dest->st_mode |= S_IFDIR;

    return 0;
}


static size_t      dir_read(    struct uufile *f, void *dest, size_t bytes)
{
    assert(f->flags & UU_FILE_FLAG_DIR);
    return common_dir_read(f, dest, bytes);
}

static size_t      dir_write(   struct uufile *f, const void *src, size_t bytes)
{
    (void) f;
    (void) src;
    (void) bytes;

    return -1;
}


static size_t      dir_getpath( struct uufile *f, void *dest, size_t bytes)
{
    assert(f->flags & UU_FILE_FLAG_DIR);
    if( bytes == 0 ) return 0;
    // TODO get mountpoint
    strncpy( dest, f->name, bytes );
    return strlen(dest);
}

static ssize_t     dir_getsize( struct uufile *f)
{
    (void) f;

    return -1;
}




// -----------------------------------------------------------------------
// Interace
// -----------------------------------------------------------------------


int common_dir_read(struct uufile *f, void *dest, size_t bytes)
{
    SHOW_FLOW( 11, "Read dir for %d bytes, pos %d", bytes, f->pos );

    struct dirent r;
    if( bytes < sizeof(struct dirent) )
        return -1;

    char namebuf[FS_MAX_PATH_LEN];
    if( get_dir_entry_name( f, f->pos++, namebuf ) )
        return 0;

    SHOW_FLOW( 7, "Read dir pos %d = '%s'", f->pos-1, namebuf );

    r.d_ino = -1; // not 0 for some programs treat 0 as no entry
    r.d_reclen = 0;
    strlcpy( r.d_name, namebuf, FILENAME_MAX );
    r.d_namlen = strlen( r.d_name );

    *((struct dirent*)dest) = r;

    return sizeof(struct dirent);
}



#endif // HAVE_UNIX

