/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Phantom's native disk structures.
 *
 *
**/

#ifndef DISK_STRUCT_H
#define DISK_STRUCT_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


#include <phantom_types.h>




#define PHANTOM_PARTITION_TYPE_ID 0xD3

// We leave 16 blocks for bootloader
#define PHANTOM_DEFAULT_DISK_START 0x10

// Disk blocks (and mem pages) are of this size
#define DISK_STRUCT_BS 4096

#define DISK_STRUCT_VERSION_MINOR 0x0003u
#define DISK_STRUCT_VERSION_MAJOR 0x0001u
#define DISK_STRUCT_VERSION (DISK_STRUCT_VERSION_MINOR | (DISK_STRUCT_VERSION_MAJOR << 16) )


#define DISK_STRUCT_MAGIC_SUPERBLOCK		0xC001AC1D
#define DISK_STRUCT_MAGIC_SUPER_2		0xB0B0ADAD
#define DISK_STRUCT_MAGIC_FREEHEAD		0xC001FBFB
#define DISK_STRUCT_MAGIC_CONST_LIST		0xC001CBCB
#define DISK_STRUCT_MAGIC_SNAP_LIST		0xC001C0C0
#define DISK_STRUCT_MAGIC_BAD_LIST		0xC001BAD0
#define DISK_STRUCT_MAGIC_LOG_LIST		0xC001100C
#define DISK_STRUCT_MAGIC_PROGRESS_PAGE		0xDADADADA
#define DISK_STRUCT_MAGIC_BOOT_MODULE		0xC001B001
#define DISK_STRUCT_MAGIC_BOOT_LOADER		0xB001B001
#define DISK_STRUCT_MAGIC_BOOT_KERNEL		0xB001AC1D

#define DISK_STRUCT_SB_SYSNAME_SIZE		64
#define DISK_STRUCT_BM_NAME_SIZE                512

#define DISK_STRUCT_N_MODULES 			30

// Numbers of blocks to look for superblock copies in - usually first 3 are used
#define DISK_STRUCT_SB_OFFSET_LIST { 0x10, 0x100, 0x220, 0x333 }


// TODO: replace long with 64 bit int


// There are 3 copies of SB. 

typedef struct phantom_disk_superblock
{
	u_int32_t 			magic; 		// DISK_STRUCT_MAGIC_SUPERBLOCK
	u_int32_t 			version;	// DISK_STRUCT_VERSION, fail if major is incorrect

	u_int32_t 			checksum;	// Simple sum of all the superblock bytes with this field is set to zero

	u_int32_t 			blocksize;	// Ignored, must be 4096

	disk_page_no_t			sb2_addr;	// address of 2nd copy or 0 if no.
	disk_page_no_t			sb3_addr;	// address of 3rd copy or 0 if no.

	disk_page_no_t			disk_start_page; // num of 1st page we can access
	disk_page_no_t			disk_page_count; // num of 1st unavail page.

	disk_page_no_t			free_start;	// number of the first block that was never allocated. This and following blocks are not in any list and are free.
	disk_page_no_t			free_list;	// free list head or 0 if no.

	char				fs_is_clean; 	// don't need to fsck
	char				general_flags_1;	// undefined yet
	char				general_flags_2;	// undefined yet
	char				general_flags_3;	// undefined yet

	disk_page_no_t			last_snap;	// the latest snapshot or 0 if no.
	//long				last_snap_time; // unix time_t - NOT IMPL
	//long				last_snap_crc32;	// to make sure it is correct - NOT IMPL
	u_int32_t			last_snap_time; // unix time_t - NOT IMPL
	u_int32_t			last_snap_crc32;	// to make sure it is correct - NOT IMPL

	disk_page_no_t			prev_snap;	// previous snapshot or 0 if no.
	u_int32_t			prev_snap_time; // unix time_t - NOT IMPL
	u_int32_t			prev_snap_crc32;	// to make sure it is correct - NOT IMPL

	u_int32_t	 		magic2;		// 32 bits  - DISK_STRUCT_MAGIC_SUPER_2

	disk_page_no_t			boot_list;			// List of blocks with bootloader image or 0.
	disk_page_no_t			kernel_list;		// List of blocks with kernel image or 0.
	disk_page_no_t			boot_progress_page;	// Boot process is updating this page to track boot progress and recover from hangs - NOT IMPL
	disk_page_no_t			boot_module[DISK_STRUCT_N_MODULES];	// List of blocks for boot time modules, up to 32.

	u_int32_t      			last_boot_time; // time_t - for boot to prefer the latest - NOT IMPL

	disk_page_no_t			bad_list;	// List of bad blocks or 0. - NOT IMPL
	disk_page_no_t			log_list;	// List of blocks for kernel debug logging. - NOT IMPL

	char				sys_name[DISK_STRUCT_SB_SYSNAME_SIZE];	// 0-terminated description for bootloader menu

	u_int32_t      			object_space_address;	// Object space expects to be loaded here

	// unused for now
	disk_page_no_t			last_short_journal_blocks[64]; //  - NOT IMPL
	disk_page_no_t			last_long_journal_root; //  - NOT IMPL

	unsigned char			last_short_journal_flags; //  - NOT IMPL
	unsigned char			last_long_journal_flags; //  - NOT IMPL

	disk_page_no_t			prev_short_journal_blocks[64]; //  - NOT IMPL
	disk_page_no_t			prev_long_journal_root; //  - NOT IMPL

	unsigned char			prev_short_journal_flags; //  - NOT IMPL
	unsigned char			prev_long_journal_flags; //  - NOT IMPL

} phantom_disk_superblock;

void phantom_disk_format( struct phantom_disk_superblock *sb, unsigned int n_pages, const char *sysname );
int phantom_calc_sb_checksum( struct phantom_disk_superblock *sb );
int superblocks_are_equal(const struct phantom_disk_superblock *a, const struct phantom_disk_superblock *b);

void phantom_dump_superblock(phantom_disk_superblock *sb);





//
// This is designed to make disk formatting to be an EXTREMELY simple task.
//
// The simplest disk has just one block: super.  It looks like this:
// 		sb2/sb3/free_list/*snap are zero (unallocated).
// 		disk_page_count is correct. free_start is right after super.
// 		free_list_is_clean is 1. version is correct.
//
// In addition any format-time allocations can be done using free_start only.
//
// On any write access system will create free list ('cause it is not  clean) 
// and move some blocks to it to let itself to allocate 2nd/3rd superblocks.
//
// The simplest way of system installation is to put on disk kernel and boot
// modules which have object initialization data in them so that kernel
// will be loaded with those modules and will use them to create startup
// system objects. On the other hand we can bring a snap with distributive
// and it will let us to start up VERY quickly.
//
// The most interesting way to install is to have kernel to load pages from
// CDROM-based snap and write them to disk on the fly as it works. To have
// this done we just need it to read blocks from CDROM snap when they're 
// missing on HD.
//


// Boot module head

struct phantom_boot_module_head
{
	u_int32_t 			magic; 		// DISK_STRUCT_MAGIC_BOOT_MODULE

        u_int32_t                       size;           // Exact size of data that follow this header
	u_int32_t			general_flags_1;	// undefined yet
	u_int32_t			_reserved;	// undefined yet

	char				name[DISK_STRUCT_BM_NAME_SIZE];	// 0-terminated module name

};


// This is general block list

#define N_REF_PER_BLOCK ( (DISK_STRUCT_BS/sizeof(disk_page_no_t)) - sizeof(struct phantom_disk_blocklist_head))

struct phantom_disk_blocklist_head
{
	u_int32_t			magic; 		// 32 bits  - DISK_STRUCT_MAGIC_FREEHEAD for freelist
	u_int32_t			used;		// num of first unused slot in list
	disk_page_no_t			next;		// next list page in a chain or 0
	disk_page_no_t			_reserved;	// for future...
};


struct phantom_disk_blocklist
{
	struct phantom_disk_blocklist_head		head;
	disk_page_no_t					list[N_REF_PER_BLOCK];
};


//
// NOT USED YET
//
// Boot progress page. This disk page is used to track boot progress.
// Kernel updates this page during the boot process and uses the tracking 
// data to prevent boot problems on the second try after lockup.
//

struct phantom_disk_boot_progress_page
{
	u_int32_t               magic; 			// 32 bits  - DISK_STRUCT_MAGIC_PROGRESS_PAGE
	u_int32_t               boot_retry;		// Number of boot try
	u_int32_t             	checksum;		// ?
	u_int32_t     	        _reserved;		// ?

	// General status flags
	unsigned char		boot_success;	// System boot up well
	unsigned char		shut_success;	// System done shutdown well

	// Boot steps flags
	unsigned char		boot_kernel_disk_access;	// Set as disk access is gained
	unsigned char		boot_kernel_bkthreads;		// Basic kernel threads are running
	unsigned char		boot_kernel_svga;			// SVGA driver is switched mode
	unsigned char		boot_kernel_splash;			// Splash demo is shown
	unsigned char		boot_kernel_paging;			// Paging is started
	
	// Shutdown	

};



#ifdef __cplusplus
//extern "C" {
}
#endif //__cplusplus


#endif // DISK_STRUCT_H
