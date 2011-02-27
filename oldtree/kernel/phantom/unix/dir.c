#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "devfs"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <unix/uufile.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <queue.h>
#include <hal.h>



typedef struct dir_impl
{
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
    }
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

#endif // HAVE_UNIX

