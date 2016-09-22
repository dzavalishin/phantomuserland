/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * File System Check.
 *
 **/

#include "cpfs_fsck.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>

#include <stdarg.h>

#include "cpfs.h"
#include "cpfs_local.h"

#include <fcntl.h>

#define MAX_FOUND_FN 32

const char* lost_found = "lost+found";

typedef enum {
    msg, warn1, err1
} severity_t; //VE macos reports an error for warn1, err1.


static void fslog (cpfs_fs_t *fs, severity_t severity, const char *fmt, ...);

static errno_t fsck_sb (cpfs_fs_t *fs, int fix);
static void fsck_scan_dirs (cpfs_fs_t *fs);


static void fsck_scan_ino (cpfs_fs_t *fs);

void fsck_log_inode (FILE *file, cpfs_blkno_t phys_blk, int ino_in_blk, struct cpfs_inode inode);

void fsck_log_de (FILE *file, cpfs_blkno_t phys_blk, int ino_in_blk, struct cpfs_dir_entry *de);

void fsck_log_block (FILE* file, cpfs_blkno_t phys_blk, fsck_blkstate_t state);

errno_t fsck_update_block_maps (cpfs_fs_t *fs, struct cpfs_inode inode_copy, fsck_blkstate_t state);

errno_t fsck_update_block_map (cpfs_fs_t *fs, cpfs_blkno_t blk, fsck_blkstate_t state);

errno_t fsck_update_ino_map (cpfs_fs_t *fs, cpfs_ino_t ino, fsck_inostate_t state);

errno_t fsck_check_block_map (cpfs_fs_t *fs);

void fsck_find_lost_ino (cpfs_fs_t *fs);

errno_t
cpfs_fsck (cpfs_fs_t *fs, int fix) {
    errno_t rc;

    cpfs_assert(fs->disk_size < INT_MAX); // TODO document

    fs->fsck_nWarn = 0;
    fs->fsck_nErr = 0;
    fs->fsck_rebuild_free = 0;

    fs->fsck_blk_state = calloc(sizeof ( fsck_blkstate_t), fs->disk_size);
    if (0 == fs->fsck_blk_state) return ENOMEM;

    fs->fsck_ino_state = calloc(sizeof ( fsck_inostate_t), fs->sb.ninode);
    if (fs->fsck_ino_state == 0) return ENOMEM;

    rc = fsck_sb(fs, fix);
    if (rc) goto error;

    fsck_scan_dir_log_file = fopen("fsck_scan_dir.log", "wb");

    fsck_scan_dirs(fs);

    fsck_find_lost_ino(fs);

    fclose(fsck_scan_dir_log_file);

    rc = fsck_check_block_map(fs);
    if (rc) goto error;


    /*
        fsck_scan_ino_log_file = fopen("fsck_scan_ino.log", "wb");
        fsck_scan_ino(fs);
        fclose(fsck_scan_ino_log_file);
     */


    printf("FSCK done, %d warnings, %d errors\n", fs->fsck_nWarn, fs->fsck_nErr);

    return (fs->fsck_nErr > 0) ? EINVAL : 0;

error:
    cpfs_log_error("FSCK error: %d", rc);
    return rc;
}

// TODO fsck is not finished

static errno_t
fsck_sb (cpfs_fs_t *fs, int fix) {
    //errno_t rc;

    printf("FSCK - ckeck superblock\n");

    struct cpfs_sb *sb = cpfs_lock_blk(fs, 0);
    if (sb == 0) return EIO;



    if (sb->h.magic != CPFS_SB_MAGIC) {
        fslog(fs, err1, "SB magic 0 wrong, %x", sb->h.magic);
    }


    if (sb->ninode < 1024) {
        fslog(fs, err1, "SB ninode too small %d", sb->ninode);
    }

    if (sb->itable_pos != 1) {
        fslog(fs, err1, "SB itable_pos != 1, %lld", sb->itable_pos);
    }

    if (sb->itable_end < sb->itable_pos) {
        fslog(fs, err1, "SB itable_end < itable_pos, %lld", sb->itable_end);
    }

    int ino_table_blkno = CPFS_INO_PER_BLK * sb->ninode;

    if (sb->itable_end >= sb->itable_pos + ino_table_blkno) {
        fslog(fs, err1, "SB sb->itable_end >= sb->itable_pos+ino_table_blkno, %lld", sb->itable_end);
    }

    if (sb->disk_size != fs->disk_size) {
        fslog(fs, err1, "SB disk_size != fs->disk_size, %lld", sb->disk_size);
    }


    /*
    sb->first_unallocated = ino_table_blkno+sb->itable_pos;
    sb->free_list = 0;

    sb->free_count = disk_size;
    sb->free_count -= 1; // sb
    sb->free_count -= ino_table_blkno;

    // sanity check
    if( sb->free_count != disk_size - sb->first_unallocated )
        cpfs_panic("sb->free_count (%lld) != disk_size (%lld) - sb->first_unallocated (%lld)", (long long)sb->free_count, (long long)disk_size, (long long)sb->first_unallocated);

    if( sb->first_unallocated >= disk_size )
    {
        cpfs_unlock_blk( fs, sb_blk );
        return EINVAL;
    }


     */
    //cpfs_touch_blk( fs, sb_blk ); // marks block as dirty, will be saved to disk on unlock
    cpfs_unlock_blk(fs, 0);
    /*
    // temp init global superblock copy for lock_ino to work
    fs->sb = *sb;


    cpfs_ino_t root_dir = 0;

    // Init inode 0 as root dir

    struct cpfs_inode *rdi = cpfs_lock_ino( fs, root_dir );
    cpfs_touch_ino( fs, root_dir );
    cpfs_inode_init_defautls( fs, rdi );
    rdi->ftype = CPFS_FTYPE_DIR;
    rdi->nlinks = 1;
    cpfs_unlock_ino( fs, root_dir );

    // de-init!
    memset( &fs->sb, 0, sizeof( fs->sb ) );
     */





    return 0;

}

void
fslog (cpfs_fs_t *fs, severity_t severity, const char *fmt, ...) {
    switch (severity) {
        case msg:
            printf("FSCK Info: ");
            break;

        case warn1:
            printf("FSCK Warn: ");
            fs->fsck_nWarn++;
            break;

        case err1:
            printf("FSCK Err:  ");
            fs->fsck_nErr++;
            break;
    }

    va_list ptr;
    va_start(ptr, fmt);
    vprintf(fmt, ptr);
    va_end(ptr);

    printf("\n");
}




// -----------------------------------------------------------------------
//
// Scan dir tree
//
// -----------------------------------------------------------------------
#define FSCK_VERBOSE 0

static errno_t fsck_scan_dir (cpfs_fs_t *fs, cpfs_ino_t dir, size_t depth);

static dir_scan_ret_t
de_log (cpfs_fs_t *fs, cpfs_blkno_t phys_blk, cpfs_ino_t offset, struct cpfs_dir_entry *de, void *farg) {

    if (de->inode == 0) return dir_scan_continue;

    fsck_log_de(fsck_scan_dir_log_file, phys_blk, offset, de);

    return dir_scan_continue;
}

static dir_scan_ret_t
de_subscan (cpfs_fs_t *fs, cpfs_blkno_t phys_blk, cpfs_ino_t offset, struct cpfs_dir_entry *de, void *farg) {
    errno_t rc;
    size_t depth = (size_t) farg;

    if (de->inode == 0) return dir_scan_continue;

    //if( 0 == strcmp( (const char*)farg, de->name ) )        return dir_scan_error;

    if (FSCK_VERBOSE) {
        int i;
        for (i = depth; i; i--)
            printf("\t");
        printf("%s", de->name);
    }

    struct cpfs_inode *inode_p = cpfs_lock_ino(fs, de->inode);
    struct cpfs_inode inode_copy = *inode_p;
    cpfs_unlock_ino(fs, de->inode);
    int isdir = (inode_copy.ftype == CPFS_FTYPE_DIR);

    if (TRACE) {
        fsck_log_de(fsck_scan_dir_log_file, phys_blk, offset, de);
        int pos = de->inode % CPFS_INO_PER_BLK;
        cpfs_blkno_t disk_block = de->inode / CPFS_INO_PER_BLK + 1;
        fsck_log_inode(fsck_scan_dir_log_file, disk_block, pos, inode_copy);
    }

    rc = fsck_update_block_maps(fs, inode_copy, bs_allocated);
    if (rc) {
        cpfs_panic("inode=%lld uses already allocated block(s).", (long long) de->inode); //debug
    }

    rc = fsck_update_ino_map(fs, de->inode, is_used);
    if (rc) {
        cpfs_panic("more than one use of inode=%lld", (long long) de->inode); //debug
    }

    if (isdir) {
        if (FSCK_VERBOSE) printf("/\n");

        rc = fsck_scan_dir(fs, de->inode, depth + 1);
        if (rc) return dir_scan_error;
    } else {
        int stop_line = 1; //for debug only.
        (void) stop_line;
        if (FSCK_VERBOSE) printf("\n");
    }


    return dir_scan_continue;
}

errno_t
fsck_scan_dir (cpfs_fs_t *fs, cpfs_ino_t dir, size_t depth) {
    //put block 
    cpfs_blkno_t blk;
    errno_t rc = cpfs_block_4_inode(fs, dir, &blk);
    if (rc) cpfs_panic("cpfs_block_4_inode: %lld", (long long) dir);

    rc = fsck_update_block_map(fs, blk, bs_inode);

    if (TRACE) {
        //printf("scan inode=%lld, block=%lld, offset=%lld \n", (long long) dir, (long long) blk, (long long) (dir % CPFS_INO_PER_BLK));
        struct cpfs_inode *inode_p = cpfs_lock_ino(fs, dir);
        struct cpfs_inode inode = *inode_p;
        cpfs_unlock_ino(fs, dir);

        fsck_log_inode(fsck_scan_dir_log_file, blk, dir % CPFS_INO_PER_BLK, inode);
    }

    rc = cpfs_scan_dir(fs, dir, de_subscan, (void *) depth);
    return rc;
}

const char*
getInoStateName (fsck_inostate_t state) { //is_unknown, is_used
    switch (state) {
        case is_unknown: return "is_unknown";
        case is_used: return "is_used";
    }
}

const char*
getBlkStateName (fsck_blkstate_t state) { //bs_unknown, bs_allocated, bs_freelist, bs_freemap, bs_inode, bs_superblk
    switch (state) {
        case bs_unknown: return "bs_unknown";
        case bs_allocated: return "bs_allocated";
        case bs_freelist: return "bs_freelist";
        case bs_freemap: return "bs_freemap";
        case bs_inode: return "bs_inode";
        case bs_superblk: return "bs_superblk";
    }
}

void
fsck_find_lost_ino (cpfs_fs_t *fs) {
    //test. one inode was found
    errno_t rc;
    cpfs_ino_t lost_found_dir_ino;

    int create_lost_found_dir = 1; //1

    printf("unused inodes:\n");
    for (cpfs_ino_t ino = 0; ino < fs->sb.ninode; ino++) {
        if (fs->fsck_ino_state[ino] == is_unknown) {

            struct cpfs_inode *inode_p = cpfs_lock_ino(fs, ino);
            struct cpfs_inode inode = *inode_p;
            cpfs_unlock_ino(fs, ino);

            if (TRACE) {
                //printf("ino=%lld, state=%s \n", (long long) ino, getInoStateName(fs->fsck_ino_state[ino]));
                fsck_log_inode(fsck_scan_dir_log_file, 1 + ino / CPFS_INO_PER_BLK, ino % CPFS_INO_PER_BLK, inode);
            }

            if (!inode.nlinks) {
                continue;
            }
            //write log.

            if (create_lost_found_dir) {
                rc = cpfs_mkdir(fs, lost_found, 0); //ENOSPC=28 (not enough free space ), EEXIST=17 (exists!)

                cpfs_assert(rc != ENOSPC); //пока так.
                if (rc) {
                    printf("mkdir: %s dir. code=%d \n", lost_found, rc);
                } else {
                }

                rc = cpfs_namei(fs, 0, lost_found, &lost_found_dir_ino);
                create_lost_found_dir = 0;
                if (TRACE) {
                    rc = cpfs_scan_dir(fs, 0, de_log, 0);
                }
            }

            //create dir_entry for lost inode.
            //no need to check is this dir or file ?
            //int isdir;
            //rc = cpfs_is_dir(fs, ino, &isdir); //TODO check rc ?

            char file_name[MAX_FOUND_FN];
            snprintf(file_name, MAX_FOUND_FN, "found_%d", ino);

            rc = cpfs_namei(fs, 0, lost_found, &lost_found_dir_ino);
            rc = cpfs_alloc_dirent(fs, lost_found_dir_ino, file_name, ino);

            if (TRACE) {
                struct cpfs_inode *lost_dir_inode_p = cpfs_lock_ino(fs, lost_found_dir_ino);
                struct cpfs_inode lost_dir_inode = *lost_dir_inode_p;
                cpfs_unlock_ino(fs, lost_found_dir_ino);
                fsck_log_inode(fsck_scan_dir_log_file, 1 + lost_found_dir_ino / CPFS_INO_PER_BLK, lost_found_dir_ino % CPFS_INO_PER_BLK, lost_dir_inode);
                //fsck_scan_dir(fs, lost_found_dir_ino, 0);
                rc = cpfs_scan_dir(fs, lost_found_dir_ino, de_log, 0);
            }
        }
    }

    int stop = 1;

}

static void
fsck_scan_dirs (cpfs_fs_t *fs) {
    printf("fsck scan dirs\n");

    fsck_update_ino_map(fs, 0, is_used); //init

    fsck_scan_dir(fs, 0, 0);

    if (TRACE) {
        /*
                printf("bs_unknown blocks:\n");
                for (cpfs_blkno_t blk = 0; blk < fs->disk_size; blk++) {
                    if (fs->fsck_blk_state[blk] == bs_unknown) {
                        printf("blk=%lld, state=%s \n", (long long) blk, getBlkStateName(fs->fsck_blk_state[blk]));
                    }
                }
                printf("press any key\n");
                getchar();
         */
    }

}

static void
fsck_scan_ino (cpfs_fs_t *fs) {
    printf("fsck scan all i-nodes START\n");

    cpfs_blkno_t itable_end = fs->sb.itable_end;
    cpfs_blkno_t itable_pos = fs->sb.itable_pos;

    for (cpfs_blkno_t disk_block = itable_pos; disk_block < itable_end; disk_block++) {
        for (int pos = 0; pos < CPFS_INO_PER_BLK; pos++) {

            cpfs_ino_t ino = (disk_block - 1) * CPFS_INO_PER_BLK + pos;
            //get inode info.
            struct cpfs_inode *inode_p = cpfs_lock_ino(fs, ino);
            struct cpfs_inode inode = *inode_p;
            cpfs_unlock_ino(fs, ino);

            if (TRACE) {
                //printf("scan inode=%lld", (long long) ino);
                fsck_log_inode(fsck_scan_ino_log_file, disk_block, pos, inode);
            }

            errno_t rc = fsck_update_ino_map(fs, ino, is_used);
            if (rc) {
                cpfs_panic("more than one use of inode=%lld", ino);
            }
            //VE is this possible?
            if (inode.nlinks > 1) {
                if (TRACE) {
                    printf(" bad\n");
                }

            } else {
                if (TRACE) {
                    printf(" OK\n");
                }
            }



        }
    }

    printf("fsck scan all i-nodes DONE\n");
}

errno_t
fsck_update_block_maps (cpfs_fs_t *fs, struct cpfs_inode inode_copy, fsck_blkstate_t state) {
    errno_t rc = 0;
    for (int i = 0; i < CPFS_INO_DIR_BLOCKS && inode_copy.blocks0[i] != 0; i++) {
        rc += fsck_update_block_map(fs, inode_copy.blocks0[i], state);
        fsck_log_block(fsck_scan_dir_log_file, inode_copy.blocks0[i], state);
    }
    //TODO indir

    //TODO indir
    return rc;
}

errno_t
fsck_update_ino_map (cpfs_fs_t *fs, cpfs_ino_t ino, fsck_inostate_t state) {

    cpfs_mutex_lock(fs->sb_mutex);

    cpfs_assert(ino < fs->sb.ninode);
    if (fs->fsck_ino_state[ino] == is_used) {
        printf("++++++++++++ %lld\n", (long long) ino);
        return 1; //кто-то повторно ссылается на иноду
    }
    fs->fsck_ino_state[ino] = state;

    cpfs_mutex_unlock(fs->sb_mutex);
    return 0;
}


// -----------------------------------------------------------------------
//
// Update/check block map
//
// -----------------------------------------------------------------------

errno_t
fsck_update_block_map (cpfs_fs_t *fs, cpfs_blkno_t blk, fsck_blkstate_t state) {
    cpfs_assert(blk < fs->disk_size); // TODO or superblock disk size? Or shall we check sb before?

    if (fs->fsck_blk_state[blk] != bs_unknown) {
        if ((state == bs_freelist) || (state == bs_freemap)) {
            fslog(fs, err1, "block %lld state was %d, attempt to set free\n", blk, fs->fsck_blk_state[blk]);

            fs->fsck_blk_state[blk] = state;
            fs->fsck_rebuild_free = 1;
            return EBUSY;
        } else if ((fs->fsck_blk_state[blk] == bs_freelist) || (fs->fsck_blk_state[blk] == bs_freemap)) {
            fslog(fs, err1, "block %lld state was free, attempt to set %d\n", blk, state);

            fs->fsck_blk_state[blk] = state;
            fs->fsck_rebuild_free = 1;
            return EBUSY;
        } else if (fs->fsck_blk_state[blk] != bs_inode) {
            //
            fslog(fs, err1, "block %lld state %d, attempt to set %d\n", blk, fs->fsck_blk_state[blk], state);
            // TODO and what?
            return EBUSY;
        }
    }

    fs->fsck_blk_state[blk] = state;
    return 0;
}

/**
 *
 * Scan through map of disk block states, check that state is correct.
 *
 **/

errno_t
fsck_check_block_map (cpfs_fs_t *fs) {
    cpfs_blkno_t blk;

    // no reason to check :)
    //if( blk[0] ==

    int iblocks = fs->sb.ninode / CPFS_INO_PER_BLK;
    cpfs_blkno_t ilast = fs->sb.itable_pos + iblocks;

    fsck_blkstate_t *bs = fs->fsck_blk_state;

    for (blk = 1; blk < fs->disk_size; blk++) {
        if (blk < ilast) {
            if ((bs[blk] != bs_inode) && (bs[blk] != bs_unknown)) {
                fslog(fs, err1, "blk %lld is not inode (%d)", blk, bs[blk]);
            }
        } else {
            if (bs[blk] != bs_unknown) {
                //TODO what?
            }
        }

    }

    return 0;
}

void
fsck_log_block (FILE* file, cpfs_blkno_t phys_blk, fsck_blkstate_t state) {
    fprintf(file, "blk=%lld, pos=-1, state=%d  \n", (long long) phys_blk, state);
    fflush(file);
}

void
fsck_log_de (FILE* file, cpfs_blkno_t phys_blk, int ino_in_blk, struct cpfs_dir_entry* de) {
    fprintf(file, "blk=%lld, pos=%d, data[ino=%lld, name=%s]  \n", (long long) phys_blk, ino_in_blk, (long long) de->inode, de->name);
    fflush(file);
}

void
fsck_log_inode (FILE *file, cpfs_blkno_t phys_blk, int ino_in_blk, struct cpfs_inode inode) {
    int is_dir = (inode.ftype == CPFS_FTYPE_DIR);
    fprintf(file, "blk=%lld, pos=%d, data[isdir=%d, fileSize=%lld, nlink=%u, first block=%lld] blocks0[", phys_blk, ino_in_blk, is_dir, (long long) inode.fsize, inode.nlinks, (long long) inode.blocks0[0]);
    for (int idx = 0; idx < CPFS_INO_DIR_BLOCKS && inode.blocks0[idx] != 0; idx++) {
        fprintf(file, "%s%lld", idx == 0 ? "" : ", ", (long long) inode.blocks0[idx]);
    }

    fprintf(file, "] indir[");
    for (int idx = 0; idx < CPFS_MAX_INDIR && inode.indir[idx] != 0; idx++) {
        fprintf(file, "%s%lld", idx == 0 ? "" : ", ", (long long) inode.indir[idx]);
    }
    fprintf(file, "] \n");
    fflush(file);
}
