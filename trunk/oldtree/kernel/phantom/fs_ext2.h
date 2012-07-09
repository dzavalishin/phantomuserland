
#define __u32 u_int32_t
#define __u16 u_int16_t
#define __u8  u_int8_t

#define TRUE 1
#define FALSE 0

#define vm_deallocate(who,p,sz) free((void *)p)
#define vm_allocate(who,pp,sz,flg) ( (  (*((void **)(pp))) = calloc(sz,1) ) ? ENOMEM : 0 )

// FIXME must get from partition struct!
#define DEV_BSIZE 512

typedef int boolean_t;
typedef unsigned int daddr_t;
typedef int mach_port_t;
typedef int mach_msg_type_number_t;
#define kern_return_t errno_t
typedef int recnum_t;




#include <errno.h>
#include <sys/types.h>
#include <kernel/mutex.h>
#include <kernel/page.h>

#define vm_page_size PAGE_SIZE


#define	FS_NOT_DIRECTORY	ENOTDIR		/* not a directory */
#define	FS_NO_ENTRY		ENOENT		/* name not found */
#define	FS_NAME_TOO_LONG	ENAMETOOLONG	/* name too long */
#define	FS_SYMLINK_LOOP		ELOOP		/* symbolic link loop */
#define	FS_INVALID_FS		EINVAL		/* bad file system */
#define	FS_NOT_IN_FILE		EINVAL		/* offset not in file */
#define	FS_INVALID_PARAMETER	EINVAL		/* bad parameter  */.

#define	IFMT		00170000
#define	IFSOCK		0140000
#define	IFLNK		0120000
#define	IFREG		0100000
#define	IFBLK		0060000
#define	IFDIR		0040000
#define	IFCHR		0020000
#define	IFIFO		0010000
#define	ISUID		0004000
#define	ISGID		0002000
#define	ISVTX		0001000




#define EXT2_MIN_BLOCK_SIZE		1024
#define	EXT2_MAX_BLOCK_SIZE		4096
#define EXT2_MIN_BLOCK_LOG_SIZE		  10




/*
 * Special inodes numbers
 */
#define	EXT2_BAD_INO		 1	/* Bad blocks inode */
#define EXT2_ROOT_INO		 2	/* Root inode */
#define EXT2_ACL_IDX_INO	 3	/* ACL inode */
#define EXT2_ACL_DATA_INO	 4	/* ACL inode */
#define EXT2_BOOT_LOADER_INO	 5	/* Boot loader inode */
#define EXT2_UNDEL_DIR_INO	 6	/* Undelete directory inode */

/* First non-reserved inode for old ext2 filesystems */
#define EXT2_GOOD_OLD_FIRST_INO	11

/*
 * The second extended file system magic number
 */
#define EXT2_SUPER_MAGIC	0xEF53

/*
 * Maximal count of links to a file
 */
#define EXT2_LINK_MAX		32000









/*
 * File system states
 */
#define	EXT2_VALID_FS			0x0001	/* Unmounted cleanly */
#define	EXT2_ERROR_FS			0x0002	/* Errors detected */

/*
 * Mount flags
 */
#define EXT2_MOUNT_CHECK_NORMAL		0x0001	/* Do some more checks */
#define EXT2_MOUNT_CHECK_STRICT		0x0002	/* Do again more checks */
#define EXT2_MOUNT_CHECK		(EXT2_MOUNT_CHECK_NORMAL | \
					 EXT2_MOUNT_CHECK_STRICT)
#define EXT2_MOUNT_GRPID		0x0004	/* Create files with directory's group */
#define EXT2_MOUNT_DEBUG		0x0008	/* Some debugging messages */
#define EXT2_MOUNT_ERRORS_CONT		0x0010	/* Continue on errors */
#define EXT2_MOUNT_ERRORS_RO		0x0020	/* Remount fs ro on errors */
#define EXT2_MOUNT_ERRORS_PANIC		0x0040	/* Panic on errors */
#define EXT2_MOUNT_MINIX_DF		0x0080	/* Mimics the Minix statfs */

#define clear_opt(o, opt)		o &= ~EXT2_MOUNT_##opt
#define set_opt(o, opt)			o |= EXT2_MOUNT_##opt
#define test_opt(sb, opt)		((sb)->u.ext2_sb.s_mount_opt & \
					 EXT2_MOUNT_##opt)
/*
 * Maximal mount counts between two filesystem checks
 */
#define EXT2_DFL_MAX_MNT_COUNT		20	/* Allow 20 mounts */
#define EXT2_DFL_CHECKINTERVAL		0	/* Don't use interval check */

/*
 * Behaviour when detecting errors
 */
#define EXT2_ERRORS_CONTINUE		1	/* Continue execution */
#define EXT2_ERRORS_RO			2	/* Remount fs read-only */
#define EXT2_ERRORS_PANIC		3	/* Panic */
#define EXT2_ERRORS_DEFAULT		EXT2_ERRORS_CONTINUE







/*
 * Constants relative to the data blocks
 */
#define	EXT2_NDIR_BLOCKS		12
#define	EXT2_IND_BLOCK			EXT2_NDIR_BLOCKS
#define	EXT2_DIND_BLOCK			(EXT2_IND_BLOCK + 1)
#define	EXT2_TIND_BLOCK			(EXT2_DIND_BLOCK + 1)
#define	EXT2_N_BLOCKS			(EXT2_TIND_BLOCK + 1)









struct ext2_super_block {
   u_int32_t  s_inodes_count;
   u_int32_t  s_blocks_count;
   u_int32_t  s_r_blocks_count;
   u_int32_t  s_free_blocks_count;
   u_int32_t  s_free_inodes_count;
   u_int32_t  s_first_data_block;
   u_int32_t  s_log_block_size;
   int32_t    s_log_frag_size;
   u_int32_t  s_blocks_per_group;
   u_int32_t  s_frags_per_group;
   u_int32_t  s_inodes_per_group;
   u_int32_t  s_mtime;
   u_int32_t  s_wtime;
   u_int16_t  s_mnt_count;
   int16_t    s_max_mnt_count;
   u_int16_t  s_magic;
   u_int16_t  s_state;
   u_int16_t  s_errors;
   u_int16_t  s_pad;
   u_int32_t  s_lastcheck;
   u_int32_t  s_checkinterval;

   u_int32_t  s_creator_os;		/* OS */
   u_int32_t  s_rev_level;		/* Revision level */
   u_int16_t  s_def_resuid;		/* Default uid for reserved blocks */
   u_int16_t  s_def_resgid;		/* Default gid for reserved blocks */

#if 0

   /*
    * These fields are for EXT2_DYNAMIC_REV superblocks only.
    *
    * Note: the difference between the compatible feature set and
    * the incompatible feature set is that if there is a bit set
    * in the incompatible feature set that the kernel doesn't
    * know about, it should refuse to mount the filesystem.
    *
    * e2fsck's requirements are more strict; if it doesn't know
    * about a feature in either the compatible or incompatible
    * feature set, it must abort and not try to meddle with
    * things it doesn't understand...
    */
   u_int32_t	s_first_ino; 		/* First non-reserved inode */
   u_int16_t   s_inode_size; 		/* size of inode structure */
   u_int16_t	s_block_group_nr; 	/* block group # of this superblock */
   u_int32_t	s_feature_compat; 	/* compatible feature set */
   u_int32_t	s_feature_incompat; 	/* incompatible feature set */
   u_int32_t	s_feature_ro_compat; 	/* readonly-compatible feature set */
   u_int8_t	s_uuid[16];		/* 128-bit uuid for volume */
   char	s_volume_name[16]; 	/* volume name */
   char	s_last_mounted[64]; 	/* directory where last mounted */
   u_int32_t	s_algorithm_usage_bitmap; /* For compression */
   /*
    * Performance hints.  Directory preallocation should only
    * happen if the EXT2_COMPAT_PREALLOC flag is on.
    */
   u_int8_t	s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
   u_int8_t	s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
   u_int16_t	s_padding1;
   u_int32_t	s_reserved[204];	/* Padding to the end of the block */

#else

   u_int32_t  s_reserved[235];
#endif
 };



typedef struct ext2_super_block ext2_super_block_t;



/*
 * ACL structures
 */
struct ext2_acl_header	/* Header of Access Control Lists */
{
	__u32	aclh_size;
	__u32	aclh_file_count;
	__u32	aclh_acle_count;
	__u32	aclh_first_acle;
};

struct ext2_acl_entry	/* Access Control List Entry */
{
	__u32	acle_size;
	__u16	acle_perms;	/* Access permissions */
	__u16	acle_type;	/* Type of entry */
	__u16	acle_tag;	/* User or group identity */
	__u16	acle_pad1;
	__u32	acle_next;	/* Pointer on next entry for the */
					/* same inode or on next free entry */
};

/*
 * Structure of a blocks group descriptor
 */
struct ext2_group_desc
{
	__u32	bg_block_bitmap;		/* Blocks bitmap block */
	__u32	bg_inode_bitmap;		/* Inodes bitmap block */
	__u32	bg_inode_table;		/* Inodes table block */
	__u16	bg_free_blocks_count;	/* Free blocks count */
	__u16	bg_free_inodes_count;	/* Free inodes count */
	__u16	bg_used_dirs_count;	/* Directories count */
	__u16	bg_pad;
	__u32	bg_reserved[3];
};



/*
 * Macro-instructions used to manage group descriptors
 */
#ifdef __KERNEL__
# define EXT2_BLOCKS_PER_GROUP(s)	((s)->u.ext2_sb.s_blocks_per_group)
# define EXT2_DESC_PER_BLOCK(s)		((s)->u.ext2_sb.s_desc_per_block)
# define EXT2_INODES_PER_GROUP(s)	((s)->u.ext2_sb.s_inodes_per_group)
# define EXT2_DESC_PER_BLOCK_BITS(s)	((s)->u.ext2_sb.s_desc_per_block_bits)
#else
# define EXT2_BLOCKS_PER_GROUP(s)	((s)->s_blocks_per_group)
# define EXT2_DESC_PER_BLOCK(s)		(EXT2_BLOCK_SIZE(s) / sizeof (struct ext2_group_desc))
# define EXT2_INODES_PER_GROUP(s)	((s)->s_inodes_per_group)
#endif




/*
 * Structure of an inode on the disk
 */
struct ext2_inode {
	__u16	i_mode;		/* File mode */
	__u16	i_uid;		/* Owner Uid */
	__u32	i_size;		/* Size in bytes */
	__u32	i_atime;	/* Access time */
	__u32	i_ctime;	/* Creation time */
	__u32	i_mtime;	/* Modification time */
	__u32	i_dtime;	/* Deletion Time */
	__u16	i_gid;		/* Group Id */
	__u16	i_links_count;	/* Links count */
	__u32	i_blocks;	/* Blocks count */
	__u32	i_flags;	/* File flags */
	union {
		struct {
			__u32  l_i_reserved1;
		} linux1;
		struct {
			__u32  h_i_translator;
		} hurd1;
		struct {
			__u32  m_i_reserved1;
		} masix1;
	} osd1;				/* OS dependent 1 */
	__u32	i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
	__u32	i_version;	/* File version (for NFS) */
	__u32	i_file_acl;	/* File ACL */
	__u32	i_dir_acl;	/* Directory ACL */
	__u32	i_faddr;	/* Fragment address */
	union {
		struct {
			__u8	l_i_frag;	/* Fragment number */
			__u8	l_i_fsize;	/* Fragment size */
			__u16	i_pad1;
			__u32	l_i_reserved2[2];
		} linux2;
		struct {
			__u8	h_i_frag;	/* Fragment number */
			__u8	h_i_fsize;	/* Fragment size */
			__u16	h_i_mode_high;
			__u16	h_i_uid_high;
			__u16	h_i_gid_high;
			__u32	h_i_author;
		} hurd2;
		struct {
			__u8	m_i_frag;	/* Fragment number */
			__u8	m_i_fsize;	/* Fragment size */
			__u16	m_pad1;
			__u32	m_i_reserved2[2];
		} masix2;
	} osd2;				/* OS dependent 2 */
};


/*
 * Structure of a directory entry
 */
#define EXT2_NAME_LEN 255

struct ext2_dir_entry {
	__u32	inode;			/* Inode number */
	__u16	rec_len;		/* Directory entry length */
	__u16	name_len;		/* Name length */
	char	name[EXT2_NAME_LEN];	/* File name */
};

/*
 * The new version of the directory entry.  Since EXT2 structures are
 * stored in intel byte order, and the name_len field could never be
 * bigger than 255 chars, it's safe to reclaim the extra byte for the
 * file_type field.
 */
struct ext2_dir_entry_2 {
	__u32	inode;			/* Inode number */
	__u16	rec_len;		/* Directory entry length */
	__u8	name_len;		/* Name length */
	__u8	file_type;
	char	name[EXT2_NAME_LEN];	/* File name */
};
























#define	SBSIZE		EXT2_MIN_BLOCK_SIZE	/* Size of superblock */
#define	SBLOCK		((daddr_t) 1)		/* Location of superblock */

#define	MAXNAMLEN	255

#define	MAXPATHLEN	1024
#define	MAXSYMLINKS	8

#define	ROOTINO		EXT2_ROOT_INO

#define	NINDIR(fs)	EXT2_ADDR_PER_BLOCK(fs)

#define	NDADDR		EXT2_NDIR_BLOCKS

#define	NIADDR		(EXT2_N_BLOCKS - EXT2_NDIR_BLOCKS)
#define	EXT2_NIADDR	(EXT2_N_BLOCKS - EXT2_NDIR_BLOCKS)


/*
 * In-core open file.
 */
struct file {
    //oskit_blkio_t		f_bioi;		/* interface to this file */
    unsigned		f_refs;		/* reference count */

    //oskit_blkio_t		*f_blkio;	/* underlying device handle */

    addr_t		f_buf;		/* buffer for data block */
    daddr_t		f_buf_blkno;	/* block number of data block */

    size_t 		f_buf_size;

    int f_dev;
    hal_mutex_t         f_lock;

    size_t              f_size;
    //size_t              f_blksize[];

    /*
     * This union no longer has any purpose;
     * it's just a vestigial hack carried over from the Mach code.
     */
    union {
        struct {
            /* pointer to super-block */
            //struct ext2_super_block*  ext2_fs;

            /* pointer to group descriptors */
            //struct ext2_group_desc*	ext2_gd;

            /* size of group descriptors */
            //size_t		ext2_gd_size;

            /* copy of on-disk inode */
            struct ext2_inode	ext2_ic;

            /* number of blocks mapped by
             indirect block at level i */
            //u_int32_t			ext2_nindir[EXT2_NIADDR+1];

            /* buffer for indirect block at level i */
            addr_t		ext2_blk[EXT2_NIADDR];

            /* size of buffer */
            size_t		ext2_blksize[EXT2_NIADDR];

            /* disk address of block in buffer */
            daddr_t			ext2_blkno[EXT2_NIADDR];
        } ext2;
    } u;
};

#define f_fs		u.ext2.ext2_fs
//#define f_gd		u.ext2.ext2_gd
//#define f_gd_size	u.ext2.ext2_gd_size
#define i_ic		u.ext2.ext2_ic
//#define f_nindir	u.ext2.ext2_nindir
#define f_blk		u.ext2.ext2_blk
#define f_blkno		u.ext2.ext2_blkno

#define f_blksize       u.ext2.ext2_blksize


struct file_direct
{
    int fd_dev;

    int fd_blksize;

    daddr_t * fd_blocks;
    int fd_size;

    int fd_bshift;
    int fd_bsize;

    int fd_fsbtodb;
};




#define EXT2_ACLE_PER_BLOCK(s)		(EXT2_BLOCK_SIZE(s) / sizeof (struct ext2_acl_entry))
#define	EXT2_ADDR_PER_BLOCK(s)		(EXT2_BLOCK_SIZE(s) / sizeof (u_int32_t))

//# define EXT2_BLOCK_SIZE(s)		((s)->s_blocksize)
# define EXT2_BLOCK_SIZE(s)		(EXT2_MIN_BLOCK_SIZE << (s)->s_log_block_size)
























#define EXT2_INODES_PER_BLOCK(fs) \
	(EXT2_INODES_PER_GROUP(fs) / EXT2_BLOCKS_PER_GROUP(fs))



static inline int
ino2blk (struct ext2_super_block *fs, struct ext2_group_desc *gd, int ino)
{
    int group;
    int blk;

    group = (ino - 1) / EXT2_INODES_PER_GROUP(fs);
    blk = gd[group].bg_inode_table +
        (((ino - 1) % EXT2_INODES_PER_GROUP(fs)) /
         EXT2_INODES_PER_BLOCK(fs));

    return blk;
}

static inline  int
itoo (struct ext2_super_block *fs, int ino)
{
    return (ino - 1) % EXT2_INODES_PER_BLOCK(fs);
}

static inline  int
blkoff (struct ext2_super_block * fs, addr_t offset)
{
    return offset % EXT2_BLOCK_SIZE(fs);
}

static inline  int
lblkno (struct ext2_super_block * fs, addr_t offset)
{
    return offset / EXT2_BLOCK_SIZE(fs);
}

static inline  int
blksize (struct ext2_super_block *fs, struct file *fp, daddr_t file_block)
{
    (void) fp;
    (void) file_block;

    return EXT2_BLOCK_SIZE(fs);	/* XXX - fix for fragments */
}


/*
 * Turn file system block numbers into disk block addresses.
 * This maps file system blocks to device size blocks.
 */
//#define fsbtodb(fs, b)	((b) << (fs)->fs_fsbtodb)
//#define	dbtofsb(fs, b)	((b) >> (fs)->fs_fsbtodb)
// FIXME 1024 bytes blk supposed
#define fsbtodb(fs, b)	((b) << 2)
#define	dbtofsb(fs, b)	((b) >> 2)


