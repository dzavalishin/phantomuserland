/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * ext2 fs ro. derived from Mach code.
 *
**/


#define DEBUG_MSG_PREFIX "fs.ext2"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include "fs_ext2.h"

#include <string.h>
#include <stdio.h>


#define mutex_lock hal_mutex_lock
#define mutex_unlock hal_mutex_unlock
#define mutex_init(m) hal_mutex_init(m,"ext2")

/*
 * Stand-alone file reading package.
 * /
#include <device/device_types.h>
#include <device/device.h>
#include <mach/mach_traps.h>
#include <mach/mach_interface.h>
#include "file_io.h"
#include "ffs_compat.h"
*/

#include <unix/uufile.h>
#include <disk.h>


// ext 2 file state
struct e2impl
{
    ino_t  		inode_num;
    struct ext2_inode   inode;
};

typedef struct e2impl e2impl_t;



// ext 2 fs state
struct e2fs_impl
{
    phantom_disk_partition_t *  partition;

    struct ext2_super_block 	fs;

    u_int32_t			ext2_nindir[EXT2_NIADDR+1];

    /* pointer to group descriptors */
    struct ext2_group_desc*	ext2_gd;

            /* size of group descriptors */
    size_t			ext2_gd_size;
};

typedef struct e2fs_impl e2fs_impl_t;




static errno_t init_ext2( e2fs_impl_t *impl );
static errno_t read_inode(ino_t inumber, struct ext2_inode *ip, e2fs_impl_t *fi);
static errno_t ext2_name_2_inode(const char *name, e2fs_impl_t *fsi, ino_t *inumber_p);




// -----------------------------------------------------------------------
// f ops impl
// -----------------------------------------------------------------------



static size_t      ext2_read(    struct uufile *f, void *dest, size_t bytes);
static size_t      ext2_write(   struct uufile *f, const void *src, size_t bytes);
static size_t      ext2_getpath( struct uufile *f, void *dest, size_t bytes);
static ssize_t     ext2_getsize( struct uufile *f);
static void *      ext2_copyimpl( void *impl );


static struct uufileops ext2_fops =
{
    .read 	= ext2_read,
    .write 	= ext2_write,

    .getpath 	= ext2_getpath,
    .getsize 	= ext2_getsize,

    .copyimpl   = ext2_copyimpl,

    //.stat       = ext2_stat,
    //.ioctl      = ext2_ioctl,
};


static size_t      ext2_read(    struct uufile *f, void *dest, size_t bytes)
{
    (void) f;
    (void) dest;
    (void) bytes;
    return -1;
}

static size_t      ext2_write(   struct uufile *f, const void *dest, size_t bytes)
{
    (void) f;
    (void) dest;
    (void) bytes;
    return -1;
}


static size_t      ext2_getpath( struct uufile *f, void *dest, size_t bytes)
{
    (void) f;
    (void) dest;
    (void) bytes;
    /*
    if( bytes == 0 ) return 0;
    // TODO get mountpoint
    // we keep name in impl
    strncpy( dest, f->impl, bytes );
    return strlen(dest);
    */
    return 0;
}

// returns -1 for non-files
static ssize_t     ext2_getsize( struct uufile *f)
{
	(void) f;

	return -1;
}

static void *      ext2_copyimpl( void *impl )
{
    void *dest = calloc( 1, sizeof(e2impl_t) );
    memcpy( dest, impl, sizeof(e2impl_t) );
    return dest;
}










// -----------------------------------------------------------------------
// FS struct
// -----------------------------------------------------------------------


static errno_t     ext2_open(struct uufile *, int create, int write);
static errno_t     ext2_close(struct uufile *);

// Create a file struct for given path
static uufile_t *  ext2_namei(uufs_t *fs, const char *filename);

// Return a file struct for fs root
static uufile_t *  ext2_getRoot(uufs_t *fs);
static errno_t     ext2_dismiss(uufs_t *fs);


struct uufs ext2_fs =
{
    .name       = "ext2",
    .open 	= ext2_open,
    .close 	= ext2_close,
    .namei 	= ext2_namei,
    .root 	= ext2_getRoot,
    .dismiss    = ext2_dismiss,

    .impl       = 0,
};

/*
static struct uufile ext2_root =
{
    .ops 	= &ext2_fops,
    .pos        = 0,
    .fs         = &ext2_fs,
    .impl       = "/",
    .flags      = UU_FILE_FLAG_NODESTROY
};
*/

static uufs_t *ext2_create_fs( e2fs_impl_t *impl )
{
    uufs_t *ret = calloc( 1, sizeof( uufs_t ) );
    assert(ret);

    memcpy( ret, &ext2_fs, sizeof( uufs_t ) );

    ret->impl = impl;

    return ret;
}




static errno_t     ext2_open(struct uufile *f, int create, int write)
{
    (void) f;
    (void) create;
    (void) write;

    //e2impl_t *impl = f->impl;

    return ENOENT;
}

static errno_t     ext2_close(struct uufile *f)
{
    (void) f;

    //e2impl_t *impl = f->impl;

    return 0;
}



static uufile_t *mkf(ino_t inumber, e2fs_impl_t *fsi)
{
    uufile_t *ret = create_uufile();

    ret->ops = &ext2_fops;
    ret->pos = 0;
    ret->fs = &ext2_fs;
    ret->impl = calloc( 1, sizeof(e2impl_t) );
    ret->flags |= UU_FILE_FLAG_FREEIMPL;

    e2impl_t *fi = ret->impl;

    fi->inode_num = inumber;

    errno_t rc = read_inode( inumber, &fi->inode, fsi );
    if(rc)
    {
        unlink_uufile( ret );
        return 0;
    }

    return ret;
}

// Create a file struct for given path
static uufile_t *  ext2_namei(uufs_t *fs, const char *filename)
{
    e2fs_impl_t *fsi = fs->impl;

    ino_t inumber;
    errno_t rc = ext2_name_2_inode(filename, fsi, &inumber);
    if( rc )
        return 0;

    return mkf( inumber, fsi);
}

// Return a file struct for fs root
static uufile_t *  ext2_getRoot(uufs_t *fs)
{
    e2fs_impl_t *fsi = fs->impl;

    uufile_t *ret = mkf( EXT2_ROOT_INO, fsi);
    if( !ret ) return 0;

    set_uufile_name( ret, "/" );

    return ret;
}


static errno_t  ext2_dismiss(uufs_t *fs)
{
    (void) fs;
    // TODO impl
    return 0;
}







// -----------------------------------------------------------------------
// We're connected here
// -----------------------------------------------------------------------

errno_t fs_start_ext2(phantom_disk_partition_t *p)
{
    e2fs_impl_t *impl = calloc(1, sizeof(e2fs_impl_t) );
    if( !impl ) return ENOMEM;

    impl->partition = p;

    errno_t err = init_ext2( impl );

    if( !err )
    {
#if HAVE_UNIX
        char pname[FS_MAX_MOUNT_PATH];
        partGetName( p, pname, FS_MAX_MOUNT_PATH );

        uufs_t *ufs = ext2_create_fs( impl );
        if( !ufs )
        {
            SHOW_ERROR( 0, "can't create uufs for %s", pname );
        }

        if( ufs && auto_mount( pname, ufs, 0, 0, AUTO_MOUNT_FLAG_AUTORUN ) )
        {
            SHOW_ERROR( 0, "can't automount %s", pname );
        }
#endif
    }

    if( err )
    {
        free( impl );
    }

    return err;
}








static errno_t ext2_read_sb(phantom_disk_partition_t *p, ext2_super_block_t *sb)
{
    unsigned char buf[PAGE_SIZE];

    if( phantom_sync_read_sector( p, buf, 2, 2 ) )
    {
        SHOW_ERROR( 0, "%s can't read sector 2", p->name );
        return EINVAL;
    }

    ext2_super_block_t *super = (void *) buf;

    *sb = *super;
    return 0;
}




errno_t fs_probe_ext2(phantom_disk_partition_t *p)
{
    errno_t error;
    ext2_super_block_t fs;

    // Read the super block
    error = ext2_read_sb(p,&fs);
    if(error)  return error;

    if (fs.s_magic != EXT2_SUPER_MAGIC)
        return FS_INVALID_FS;

    return 0;
}




static errno_t read_fs(phantom_disk_partition_t *p, struct ext2_super_block *fs, struct ext2_group_desc  **gdp, vm_size_t *gd_size_p)
{
    errno_t error;

    //struct ext2_super_block *fs = fsp;
    //vm_offset_t buf;
    //vm_offset_t buf2;
    //mach_msg_type_number_t buf_size;
    //mach_msg_type_number_t buf2_size;
    int gd_count;
    int gd_blocks;
    int gd_size;
    int gd_location;
    int gd_sector;
    // Read the super block

    error = ext2_read_sb(p,fs);
    //error = device_read(dev, 0, (recnum_t) SBLOCK, SBSIZE,                        (char **) &buf, &buf_size);
    if(error)  return error;

    /*
     * Check the superblock
     */
    if (fs->s_magic != EXT2_SUPER_MAGIC)
        return (FS_INVALID_FS);

    /*
     * Compute the groups informations
     */
    gd_count = (fs->s_blocks_count - fs->s_first_data_block +
                fs->s_blocks_per_group - 1) / fs->s_blocks_per_group;
    gd_blocks = (gd_count + EXT2_DESC_PER_BLOCK(fs) - 1) /
        EXT2_DESC_PER_BLOCK(fs);
    gd_size = gd_blocks * EXT2_BLOCK_SIZE(fs);
    gd_location = fs->s_first_data_block + 1;
    gd_sector = (gd_location * EXT2_BLOCK_SIZE(fs)) / DEV_BSIZE;

    *gdp = calloc( 1, gd_size );
    if( !*gdp ) return ENOMEM;

    *gd_size_p = gd_size;

    /*
     * Read the groups descriptors
     */
    //error = device_read(dev, 0, (recnum_t) gd_sector, gd_size,                        (char **) &buf2, &buf2_size);
    error = phantom_sync_read_sector( p, *gdp, gd_sector, gd_blocks );
    if(error)
    {
        free( *gdp );
        return error;
    }

    //*gdp = (struct ext2_group_desc *) buf2;
    return 0;
}




static errno_t init_ext2( e2fs_impl_t *impl )
{
    //struct file *fp;
    //;
    errno_t error = read_fs( impl->partition, &impl->fs, &impl->ext2_gd, &impl->ext2_gd_size);
    if(error) return error;

    ext2_super_block_t *fs = &impl->fs;

    // Calculate indirect block levels.

    int level;
    int mult = 1;

    for (level = 0; level < NIADDR; level++)
    {
        mult *= NINDIR(fs);
        impl->ext2_nindir[level] = mult;
    }

    return 0;
}


















/*
 * Read a new inode into a file structure.
 */
static errno_t read_inode(ino_t inumber, struct ext2_inode *ip, e2fs_impl_t *fi)
{
    struct ext2_super_block *fs = &fi->fs;
    errno_t rc;
    daddr_t disk_block;

    //vm_offset_t buf;
    //mach_msg_type_number_t buf_size;

    disk_block = ino2blk(fs, fi->ext2_gd, inumber);
    /*
    rc = device_read(fp->f_dev,
                     0,
                     (recnum_t) fsbtodb(fp->f_fs, disk_block),
                     (int) EXT2_BLOCK_SIZE(fs),
                     (char **)&buf,
                     &buf_size);
                     */

    unsigned char buf[PAGE_SIZE];
    rc = phantom_sync_read_sector( fi->partition, buf, fsbtodb(fs, disk_block), EXT2_BLOCK_SIZE(fs)/512 );
    if(rc)         return rc;

    {
        struct ext2_inode *dp;
        dp = (struct ext2_inode *)buf;
        dp += itoo(fs, inumber);
        //fp->i_ic = *dp;
        //fp->f_size = dp->i_size;
        *ip = *dp;
    }
    //(void) vm_deallocate(mach_task_self(), buf, buf_size);
    /*
     * Clear out the old buffers
     */
    //free_file_buffers(fp);
    return (0);
}



#if 0
/*
 * Search a directory for a name and return its i_number.
 */
static errno_t ext2_name_2_inode(const char *name, e2fs_impl_t *fsi, ino_t *inumber_p)
{
    vm_offset_t buf;
    vm_size_t buf_size;
    vm_offset_t offset;
    struct ext2_dir_entry_2 *dp;
    int length;
    kern_return_t rc;
    char tmp_name[256];
    length = strlen(name);
    offset = 0;
    while (offset < fp->i_ic.i_size) {
        rc = buf_read_file(fp, offset, &buf, &buf_size);
        if (rc)
            return (rc);
        dp = (struct ext2_dir_entry_2 *)buf;
        if (dp->inode != 0) {
            strncpy (tmp_name, dp->name, dp->name_len);
            tmp_name[dp->name_len] = '/';
            if (dp->name_len == length &&
                !strcmp(name, tmp_name))
            {
                /* found entry */
                *inumber_p = dp->inode;
                return (0);
            }
        }
        offset += dp->rec_len;
    }
    return (FS_NO_ENTRY);
}

#endif











#if 0


void ext2_close_file(); /* forward */
/*
 * Free file buffers, but don't close file.
 */
static void free_file_buffers(struct file *fp)
{
    int level;
    /*
     * Free the indirect blocks
     */
    for (level = 0; level < NIADDR; level++) {
        if (fp->f_blk[level] != 0) {
            (void) vm_deallocate(mach_task_self(),
                                 fp->f_blk[level],
                                 fp->f_blksize[level]);
            fp->f_blk[level] = 0;
        }
        fp->f_blkno[level] = -1;
    }
    /*
     * Free the data block
     */
    if (fp->f_buf != 0) {
        (void) vm_deallocate(mach_task_self(),
                             fp->f_buf,
                             fp->f_buf_size);
        fp->f_buf = 0;
    }
    fp->f_buf_blkno = -1;
}
/*
 * Read a new inode into a file structure.
 */
static int
read_inode(inumber, fp)
ino_t inumber;
struct file *fp;
{
    vm_offset_t buf;
    mach_msg_type_number_t buf_size;
    struct ext2_super_block *fs;
    daddr_t disk_block;
    kern_return_t rc;
    fs = fp->f_fs;
    disk_block = ino2blk(fs, fp->f_gd, inumber);
    rc = device_read(fp->f_dev,
                     0,
                     (recnum_t) fsbtodb(fp->f_fs, disk_block),
                     (int) EXT2_BLOCK_SIZE(fs),
                     (char **)&buf,
                     &buf_size);
    if (rc)
        return (rc);
    {
        struct ext2_inode *dp;
        dp = (struct ext2_inode *)buf;
        dp += itoo(fs, inumber);
        fp->i_ic = *dp;
        fp->f_size = dp->i_size;
    }
    (void) vm_deallocate(mach_task_self(), buf, buf_size);
    /*
     * Clear out the old buffers
     */
    free_file_buffers(fp);
    return (0);
}
/*
 * Given an offset in a file, find the disk block number that
 * contains that block.
 */
static int
block_map(fp, file_block, disk_block_p)
struct file *fp;
daddr_t file_block;
daddr_t *disk_block_p; /* out */
{
    int level;
    int idx;
    daddr_t ind_block_num;
    kern_return_t rc;
    vm_offset_t olddata[NIADDR+1];
    vm_size_t oldsize[NIADDR+1];
    /*
     * Index structure of an inode:
     *
     * i_db[0..NDADDR-1] hold block numbers for blocks
     * 0..NDADDR-1
     *
     * i_ib[0] index block 0 is the single indirect
     * block
     * holds block numbers for blocks
     * NDADDR .. NDADDR + NINDIR(fs)-1
     *
     * i_ib[1] index block 1 is the double indirect
     * block
     * holds block numbers for INDEX blocks
     * for blocks
     * NDADDR + NINDIR(fs) ..
     * NDADDR + NINDIR(fs) + NINDIR(fs)**2 - 1
     *
     * i_ib[2] index block 2 is the triple indirect
     * block
     * holds block numbers for double-indirect
     * blocks for blocks
     * NDADDR + NINDIR(fs) + NINDIR(fs)**2 ..
     * NDADDR + NINDIR(fs) + NINDIR(fs)**2
     * + NINDIR(fs)**3 - 1
     */
    mutex_lock(&fp->f_lock);
    if (file_block < NDADDR) {
        /* Direct block. */
        *disk_block_p = fp->i_ic.i_block[file_block];
        mutex_unlock(&fp->f_lock);
        return (0);
    }
    file_block -= NDADDR;
    /*
     * nindir[0] = NINDIR
     * nindir[1] = NINDIR**2
     * nindir[2] = NINDIR**3
     * etc
     */
    for (level = 0; level < NIADDR; level++) {
        if (file_block < fp->f_nindir[level])
            break;
        file_block -= fp->f_nindir[level];
    }
    if (level == NIADDR) {
        /* Block number too high */
        mutex_unlock(&fp->f_lock);
        return (FS_NOT_IN_FILE);
    }
    ind_block_num = fp->i_ic.i_block[level + NDADDR];
    /*
     * Initialize array of blocks to free.
     */
    for (idx = 0; idx < NIADDR; idx++)
        oldsize[idx] = 0;
    for (; level >= 0; level--) {
        vm_offset_t data;
        mach_msg_type_number_t size;
        if (ind_block_num == 0)
            break;
        if (fp->f_blkno[level] == ind_block_num) {
            /*
             * Cache hit.  Just pick up the data.
             */
            data = fp->f_blk[level];
        }
        else {
            /*
             * Drop our lock while doing the read.
             * (The f_dev and f_fs fields don`t change.)
             */
            mutex_unlock(&fp->f_lock);
            rc = device_read(fp->f_dev,
                             0,
                             (recnum_t) fsbtodb(fp->f_fs, ind_block_num),
                             EXT2_BLOCK_SIZE(fp->f_fs),
                             (char **)&data,
                             &size);
            if (rc)
                return (rc);
            /*
             * See if we can cache the data.  Need a write lock to
             * do this.  While we hold the write lock, we can`t do
             * *anything* which might block for memory.  Otherwise
             * a non-privileged thread might deadlock with the
             * privileged threads.  We can`t block while taking the
             * write lock.  Otherwise a non-privileged thread
             * blocked in the vm_deallocate (while holding a read
             * lock) will block a privileged thread.  For the same
             * reason, we can`t take a read lock and then use
             * lock_read_to_write.
             */
            mutex_lock(&fp->f_lock);
            olddata[level] = fp->f_blk[level];
            oldsize[level] = fp->f_blksize[level];
            fp->f_blkno[level] = ind_block_num;
            fp->f_blk[level] = data;
            fp->f_blksize[level] = size;
            /*
             * Return to holding a read lock, and
             * dispose of old data.
             */
        }
        if (level > 0) {
            idx = file_block / fp->f_nindir[level-1];
            file_block %= fp->f_nindir[level-1];
        }
        else
            idx = file_block;
        ind_block_num = ((daddr_t *)data)[idx];
    }
    mutex_unlock(&fp->f_lock);
    /*
     * After unlocking the file, free any blocks that
     * we need to free.
     */
    for (idx = 0; idx < NIADDR; idx++)
        if (oldsize[idx] != 0)
            (void) vm_deallocate(mach_task_self(),
                                 olddata[idx],
                                 oldsize[idx]);
    *disk_block_p = ind_block_num;
    return (0);
}
/*
 * Read a portion of a file into an internal buffer.  Return
 * the location in the buffer and the amount in the buffer.
 */
static int
buf_read_file(fp, offset, buf_p, size_p)
struct file *fp;
vm_offset_t offset;
vm_offset_t *buf_p; /* out */
vm_size_t *size_p; /* out */
{
    struct ext2_super_block *fs;
    vm_offset_t off;
    daddr_t file_block;
    daddr_t disk_block;
    int rc;
    vm_offset_t block_size;
    if (offset >= fp->i_ic.i_size)
        return (FS_NOT_IN_FILE);
    fs = fp->f_fs;
    off = blkoff(fs, offset);
    file_block = lblkno(fs, offset);
    block_size = blksize(fs, fp, file_block);
    if (file_block != fp->f_buf_blkno) {
        rc = block_map(fp, file_block, &disk_block);
        if (rc != 0)
            return (rc);
        if (fp->f_buf)
            (void)vm_deallocate(mach_task_self(),
                                fp->f_buf,
                                fp->f_buf_size);
        if (disk_block == 0) {
            (void)vm_allocate(mach_task_self(),
                              &fp->f_buf,
                              block_size,
                              TRUE);
            fp->f_buf_size = block_size;
        }
        else {
            rc = device_read(fp->f_dev,
                             0,
                             (recnum_t) fsbtodb(fs, disk_block),
                             (int) block_size,
                             (char **) &fp->f_buf,
                             (mach_msg_type_number_t *)&fp->f_buf_size);
        }
        if (rc)
            return (rc);
        fp->f_buf_blkno = file_block;
    }
    /*
     * Return address of byte in buffer corresponding to
     * offset, and size of remainder of buffer after that
     * byte.
     */
    *buf_p = fp->f_buf + off;
    *size_p = block_size - off;
    /*
     * But truncate buffer at end of file.
     */
    if (*size_p > fp->i_ic.i_size - offset)
        *size_p = fp->i_ic.i_size - offset;
    return (0);
}


static void
unmount_fs(fp)
struct file *fp;
{
    if (file_is_structured(fp)) {
        (void) vm_deallocate(mach_task_self(),
                             (vm_offset_t) fp->f_fs,
                             SBSIZE);
        (void) vm_deallocate(mach_task_self(),
                             (vm_offset_t) fp->f_gd,
                             fp->f_gd_size);
        fp->f_fs = 0;
    }
}
/*
 * Open a file.
 */
int
ext2_open_file(master_device_port, path, fp)
mach_port_t master_device_port;
char * path;
struct file *fp;
{
#define RETURN(code) { rc = (code); goto exit; }
    char *cp, *component;
    int c; /* char */
    int rc;
    ino_t inumber, parent_inumber;
    int nlinks = 0;
    char namebuf[MAXPATHLEN+1];
    if (path == 0 || *path == '\0') {
        return FS_NO_ENTRY;
    }
    /*
     * Copy name into buffer to allow modifying it.
     */
    strcpy(namebuf, path);
    /*
     * Look for '/dev/xxx' at start of path, for
     * root device.
     */
    if (!strncmp(namebuf, "/dev/", 6)) {
        SHOW_ERROR0( 0, "no device name" );
        return FS_NO_ENTRY;
    }
    cp = namebuf + 5; /* device */
    component = cp;
    while ((c = *cp) != '\0' && c != '/') {
        cp++;
    }
    *cp = '\0';
    bzero (fp, sizeof (struct file));
    //rc = device_open(master_device_port,                      D_READ|D_WRITE,                      component,                     &fp->f_dev);
#error fixme
    if (rc)
        return rc;
    if (c == 0) {
        fp->f_fs = 0;
        goto out_ok;
    }
    *cp = c;
    rc = mount_fs(fp);
    if (rc)
        return rc;
    inumber = (ino_t) ROOTINO;
    if ((rc = read_inode(inumber, fp)) != 0) {
        SHOW_ERROR0( 0, "can't read root inode");
        goto exit;
    }
    while (*cp) {
        /*
         * Check that current node is a directory.
         */
        if ((fp->i_ic.i_mode & IFMT) != IFDIR)
            RETURN (FS_NOT_DIRECTORY);
        /*
         * Remove extra separators
         */
        while (*cp == '/')
            cp++;
        /*
         * Get next component of path name.
         */
        component = cp;
        {
            int len = 0;
            while ((c = *cp) != '\0' && c != '/') {
                if (len++ > MAXNAMLEN)
                    RETURN (FS_NAME_TOO_LONG);
                if (c & 0200)
                    RETURN (FS_INVALID_PARAMETER);
                cp++;
            }
            *cp = 0;
        }
        /*
         * Look up component in current directory.
         * Save directory inumber in case we find a
         * symbolic link.
         */
        parent_inumber = inumber;
        rc = search_directory(component, fp, &inumber);
        if (rc) {
            SHOW_ERROR( 0, "%s: not foundn", path );
            goto exit;
        }
        *cp = c;
        /*
         * Open next component.
         */
        if ((rc = read_inode(inumber, fp)) != 0)
            goto exit;
        /*
         * Check for symbolic link.
         */
        if ((fp->i_ic.i_mode & IFMT) == IFLNK) {
            int link_len = fp->i_ic.i_size;
            int len;
            len = strlen(cp) + 1;
            if (link_len + len >= MAXPATHLEN - 1)
                RETURN (FS_NAME_TOO_LONG);
            if (++nlinks > MAXSYMLINKS)
                RETURN (FS_SYMLINK_LOOP);
            memmove(&namebuf[link_len], cp, len);
#ifdef IC_FASTLINK
            if (fp->i_ic.i_blocks == 0) {
                bcopy(fp->i_ic.i_block, namebuf, (unsigned) link_len);
            }
            else
#endif /* IC_FASTLINK */
            {
                /*
                 * Read file for symbolic link
                 */
                vm_offset_t buf;
                mach_msg_type_number_t buf_size;
                daddr_t disk_block;
                struct ext2_super_block *fs = fp->f_fs;
                (void) block_map(fp, (daddr_t)0, &disk_block);
                rc = device_read(fp->f_dev,
                                 0,
                                 (recnum_t) fsbtodb(fs, disk_block),
                                 (int) blksize(fs, fp, 0),
                                 (char **) &buf,
                                 &buf_size);
                if (rc)
                    goto exit;
                bcopy((char *)buf, namebuf, (unsigned)link_len);
                (void) vm_deallocate(mach_task_self(), buf, buf_size);
            }
            /*
             * If relative pathname, restart at parent directory.
             * If absolute pathname, restart at root.
             * If pathname begins '/dev/<device>/',
             * restart at root of that device.
             */
            cp = namebuf;
            if (*cp != '/') {
                inumber = parent_inumber;
            }
            else if (!strprefix(cp, "/dev/")) {
                inumber = (ino_t)ROOTINO;
            }
            else {
                cp += 5;
                component = cp;
                while ((c = *cp) != '\0' && c != '/') {
                    cp++;
                }
                *cp = '\0';
                /*
                 * Unmount current file system and free buffers.
                 */
                ext2_close_file(fp);
                /*
                 * Open new root device.
                 */
                //rc = device_open(master_device_port,                                 D_READ,                                 component,                                 &fp->f_dev);
#error fixme
                if (rc)
                    return (rc);
                if (c == 0) {
                    fp->f_fs = 0;
                    goto out_ok;
                }
                *cp = c;
                rc = mount_fs(fp);
                if (rc)
                    return (rc);
                inumber = (ino_t)ROOTINO;
            }
            if ((rc = read_inode(inumber, fp)) != 0)
                goto exit;
        }
    }
    /*
     * Found terminal component.
     */
out_ok:
    mutex_init(&fp->f_lock);
    return 0;
    /*
     * At error exit, close file to free storage.
     */
exit:
    ext2_close_file(fp);
    return rc;
}
/*
 * Close file - free all storage used.
 */
void
ext2_close_file(fp)
struct file *fp;
{
    //int i;
    /*
     * Free the disk super-block.
     */
    unmount_fs(fp);
    /*
     * Free the inode and data buffers.
     */
    free_file_buffers(fp);
}
int
ext2_file_is_directory(struct file *fp)
{
    return (fp->i_ic.i_mode & IFMT) == IFDIR;
}
int
ext2_file_is_regular(struct file *fp)
{
    return (fp->i_ic.i_mode & IFMT) == IFREG;
}
/*
 * Copy a portion of a file into kernel memory.
 * Cross block boundaries when necessary.
 */
int
ext2_read_file(fp, offset, start, size, resid)
struct file *fp;
vm_offset_t offset;
vm_offset_t start;
vm_size_t size;
vm_size_t *resid; /* out */
{
    int rc;
    vm_size_t csize;
    vm_offset_t buf;
    vm_size_t buf_size;
    while (size != 0) {
        rc = buf_read_file(fp, offset, &buf, &buf_size);
        if (rc)
            return (rc);
        csize = size;
        if (csize > buf_size)
            csize = buf_size;
        if (csize == 0)
            break;
        bcopy((char *)buf, (char *)start, csize);
        offset += csize;
        start  += csize;
        size   -= csize;
    }
    if (resid)
        *resid = size;
    return (0);
}
/* simple utility: only works for 2^n */
static int
log2(n)
unsigned int n;
{
    int i = 0;
    while ((n & 1) == 0) {
        i++;
        n >>= 1;
    }
    return i;
}
/*
 * Make an empty file_direct for a device.
 */
int
ext2_open_file_direct(dev, fdp, is_structured)
mach_port_t dev;
struct file_direct *fdp;
boolean_t is_structured;
{
    struct ext2_super_block *fs;
    struct ext2_group_desc *gd;
    vm_size_t gd_size;
    int rc;
    if (!is_structured) {
        fdp->fd_dev     = dev;
        fdp->fd_blocks  = (daddr_t *) 0;
        fdp->fd_bsize   = vm_page_size;
        fdp->fd_bshift  = log2(vm_page_size);
        fdp->fd_fsbtodb = 0; /* later */
        fdp->fd_size    = 0; /* later */
        return 0;
    }
    rc = read_fs(dev, &fs, &gd, &gd_size);
    if (rc)
        return rc;
    fdp->fd_dev = dev;
    fdp->fd_blocks = (daddr_t *) 0;
    fdp->fd_size = 0;
    fdp->fd_bsize = EXT2_BLOCK_SIZE(fs);
    fdp->fd_bshift = log2(fdp->fd_bsize);
    fdp->fd_fsbtodb = log2(fdp->fd_bsize / DEV_BSIZE);
    (void) vm_deallocate(mach_task_self(),
                         (vm_offset_t) fs,
                         SBSIZE);
    (void) vm_deallocate(mach_task_self(),
                         (vm_offset_t) gd,
                         gd_size);
    return 0;
}
/*
 * Add blocks from a file to a file_direct.
 */
int
ext2_add_file_direct(fdp, fp)
struct file_direct *fdp;
struct file *fp;
{
    struct ext2_super_block *fs;
    long num_blocks, i;
    vm_offset_t buffer;
    vm_size_t size;
    int rc;
    /* the file must be on the same device */
    if (fdp->fd_dev != fp->f_dev)
        return FS_INVALID_FS;
    if (!file_is_structured(fp)) {
        /*
        int result[DEV_GET_SIZE_COUNT];
        natural_t count;
        count = DEV_GET_SIZE_COUNT;
        rc = device_get_status( fdp->fd_dev, DEV_GET_SIZE,
        result, &count);
        */
#error FIXME
        if (rc)
            return rc;
        //fdp->fd_size = result[DEV_GET_SIZE_DEVICE_SIZE] >> fdp->fd_bshift;
        //fdp->fd_fsbtodb = log2(fdp->fd_bsize/result[DEV_GET_SIZE_RECORD_SIZE]);
        return 0;
    }
    /* it must hold a file system */
    fs = fp->f_fs;
    /*
     if (fdp->fd_bsize != fs->fs_bsize ||
     fdp->fd_fsbtodb != fs->fs_fsbtodb)
     */
    if (fdp->fd_bsize != EXT2_BLOCK_SIZE(fs))
        return FS_INVALID_FS;
    /* calculate number of blocks in the file, ignoring fragments */
    num_blocks = lblkno(fs, fp->i_ic.i_size);
    /* allocate memory for a bigger array */
    size = (num_blocks + fdp->fd_size) * sizeof(daddr_t);
    rc = vm_allocate(mach_task_self(), &buffer, size, TRUE);
    if (rc)
        return rc;
    /* lookup new block addresses */
    for (i = 0; i < num_blocks; i++) {
        daddr_t disk_block;
        rc = block_map(fp, (daddr_t) i, &disk_block);
        if (rc != 0) {
            (void) vm_deallocate(mach_task_self(), buffer, size);
            return rc;
        }
        ((daddr_t *) buffer)[fdp->fd_size + i] = disk_block;
    }
    /* copy old addresses and install the new array */
    if (fdp->fd_blocks != 0) {
        bcopy((char *) fdp->fd_blocks, (char *) buffer,
              fdp->fd_size * sizeof(daddr_t));
        (void) vm_deallocate(mach_task_self(),
                             (vm_offset_t) fdp->fd_blocks,
                             (vm_size_t) (fdp->fd_size * sizeof(daddr_t)));
    }
    fdp->fd_blocks = (daddr_t *) buffer;
    fdp->fd_size += num_blocks;
    /* deallocate cached blocks */
    free_file_buffers(fp);
    return 0;
}

int
ext2_remove_file_direct(fdp)
struct file_direct *fdp;
{
    if (fdp->fd_blocks)
        (void) vm_deallocate(mach_task_self(),
                             (vm_offset_t) fdp->fd_blocks,
                             (vm_size_t) (fdp->fd_size * sizeof(daddr_t)));
    fdp->fd_blocks = 0; /* sanity */
    /* xxx should lose a ref to fdp->fd_dev here (and elsewhere) xxx */
    return 0;
}




#endif









/*
 * Portions of this code are derivef from:
 *
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
