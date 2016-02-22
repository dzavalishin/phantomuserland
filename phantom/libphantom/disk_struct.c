/**
 *
 * Phantom OS
 *
 * Phantom disk structures worker functions.
 *
 * (C) 2005-2010 dz.
 *
**/

#include <phantom_libc.h>

#include <phantom_disk.h>


static u_int32_t sb_chksum( unsigned char *sbp )
{
    u_int32_t counter = 0;
    int i;
    for( i = sizeof(phantom_disk_superblock); i--; )
        counter += *sbp++;

    return counter;
}


// Calculate and store superblock checksum. Return true if previous
// checksum is ok.

int
phantom_calc_sb_checksum( phantom_disk_superblock *sb )
{
    u_int32_t old_cs = sb->checksum;
    sb->checksum = 0; // Or else checksum will be wrong

    sb->checksum = sb_chksum( (unsigned char *)sb );

    return
        sb->checksum == old_cs &&
        sb->magic   == DISK_STRUCT_MAGIC_SUPERBLOCK &&
        sb->magic2  == DISK_STRUCT_MAGIC_SUPER_2
        ;
}

//
// Fill superblock with basic formatting info.
//
// Such superblock written to the beginning of disk

void
phantom_disk_format( phantom_disk_superblock *sb, unsigned int n_pages, const char *sysname )
{

    memset( sb, 0, sizeof(*sb) );

    sb->version = DISK_STRUCT_VERSION;
    sb->magic   = DISK_STRUCT_MAGIC_SUPERBLOCK;
    sb->magic2  = DISK_STRUCT_MAGIC_SUPER_2;
    sb->blocksize = 4096;

    sb->sb2_addr = sb->sb3_addr = 0; // No copies of superblock

    sb->disk_start_page = PHANTOM_DEFAULT_DISK_START;
    sb->disk_page_count = n_pages;

    sb->free_start = 1; // Next block after super is free.
    sb->free_list = 0;  // No free list yet.
    sb->fs_is_clean = 0xFF;

    strlcpy( sb->sys_name, sysname, sizeof(sb->sys_name) );

    phantom_calc_sb_checksum( sb );
}

#define cmpab(f) (a->f == b->f)

int superblocks_are_equal(const phantom_disk_superblock *a, const phantom_disk_superblock *b)
{
    return
        cmpab(magic) && cmpab(version) && cmpab(checksum) && cmpab(blocksize) &&
        cmpab(sb2_addr) && cmpab(sb3_addr) && cmpab(disk_start_page) &&cmpab(disk_page_count) &&
        cmpab(free_start) && cmpab(free_list) && cmpab(fs_is_clean) &&
        cmpab(general_flags_1) && cmpab(general_flags_2) && cmpab(general_flags_3) &&
        cmpab(last_snap) && cmpab(last_snap_time) && cmpab(last_snap_crc32) &&
        cmpab(prev_snap) && cmpab(prev_snap_time) && cmpab(prev_snap_crc32) &&
        cmpab(magic2) &&
        cmpab(boot_list) && //cmpab(boot_module) &&
        //cmpab(sys_name) &&
        cmpab(last_boot_time) &&
        /*cmpab(last_short_journal_blocks) && */ cmpab(last_long_journal_root) &&
        cmpab(last_short_journal_flags) && cmpab(last_long_journal_flags) &&
        /*cmpab(prev_short_journal_blocks) && */ cmpab(prev_long_journal_root) &&
        cmpab(prev_short_journal_flags) && cmpab(prev_long_journal_flags) &&
        1;
}


void
phantom_dump_superblock(phantom_disk_superblock *sb)
{
    printf("Superblock is:\n");
    printf("--------------\n");
    printf("Magic             0x%x (%s)\n", sb->magic, sb->magic == DISK_STRUCT_MAGIC_SUPERBLOCK ? "ok" : "wrong");
    printf("Magic 2           0x%x (%s)\n", sb->magic2, sb->magic2 == DISK_STRUCT_MAGIC_SUPER_2 ? "ok" : "wrong");
    printf("Version           0x%x (%s)\n", sb->version, sb->version == DISK_STRUCT_VERSION ? "ok" : "wrong");

    u_int32_t old_cs = sb->checksum;
    sb->checksum = 0;
    u_int32_t cs = sb_chksum( (unsigned char *)sb );
    sb->checksum = old_cs;

    printf("Checksum          0x%x (%s)\n", sb->checksum, sb->checksum == cs ? "ok" : "wrong");
    printf("Blocksize         0x%x (%s)\n", sb->blocksize, sb->blocksize == 4096 ? "ok" : "wrong");
    printf("--------------\n");

    printf("sb2_addr          %d\n", sb->sb2_addr );
    printf("sb3_addr          %d\n", sb->sb3_addr );
    printf("disk_start_page   %d\n", sb->disk_start_page );
    printf("disk_page_count   %d\n", sb->disk_page_count );
    printf("free_start        %d\n", sb->free_start );
    printf("free_list         %d\n", sb->free_list );
    printf("--------------\n");

    printf("fs_is_clean       %d\n", sb->fs_is_clean );
    printf("general_flags_1   0x%x\n", sb->general_flags_1 );
    printf("--------------\n");

    printf("last_snap         %d\n", sb->last_snap );
    printf("prev_snap         %d\n", sb->prev_snap );
    printf("--------------\n");

    printf("sys_name          %*s\n", DISK_STRUCT_SB_SYSNAME_SIZE, sb->sys_name );
    printf("obj spc address   0x%lx\n", (unsigned long int)sb->object_space_address );


}



/*
 * Historical junk:
 *
 * $Log: disk_struct.cpp,v $
 * Revision 1.9  2005/01/06 22:26:06  dz
 * got first page fault
 *
 * Revision 1.8  2005/01/06 21:27:18  dz
 * cleanup before first vm run
 *
 * Revision 1.7  2005/01/06 19:57:04  dz
 * Writing pager...
 *
 * Revision 1.6  2005/01/06 19:17:00  dz
 * blocksize
 */
