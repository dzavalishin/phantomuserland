package ru.dz.phantom.disk;

public class Const {

	// We leave 16 blocks for bootloader
	final public static int PHANTOM_DEFAULT_DISK_START = 0x10;
	// Disk blocks (and mem pages) are of this size
	final public static int DISK_STRUCT_BS = 4096;

	final public static int DISK_STRUCT_VERSION_MINOR = 0x0002;
	final public static int DISK_STRUCT_VERSION_MAJOR = 0x0001;
	final public static int DISK_STRUCT_VERSION = (DISK_STRUCT_VERSION_MINOR | (DISK_STRUCT_VERSION_MAJOR << 16) );
	
	final public static int DISK_STRUCT_MAGIC_SUPERBLOCK            = 0xC001AC1D;
	final public static int DISK_STRUCT_MAGIC_SUPER_2               = 0xB0B0ADAD;
	final public static int DISK_STRUCT_MAGIC_FREEHEAD              = 0xC001FBFB;
	final public static int DISK_STRUCT_MAGIC_CONST_LIST            = 0xC001CBCB;
	final public static int DISK_STRUCT_MAGIC_SNAP_LIST             = 0xC001C0C0;
	final public static int DISK_STRUCT_MAGIC_BAD_LIST              = 0xC001BAD0;
	final public static int DISK_STRUCT_MAGIC_LOG_LIST              = 0xC001100C;
	final public static int DISK_STRUCT_MAGIC_PROGRESS_PAGE         = 0xDADADADA;
	final public static int DISK_STRUCT_MAGIC_BOOT_MODULE           = 0xC001B001;
	final public static int DISK_STRUCT_MAGIC_BOOT_LOADER           = 0xB001B001;
	final public static int DISK_STRUCT_MAGIC_BOOT_KERNEL           = 0xB001AC1D;

	final public static int DISK_STRUCT_SB_SYSNAME_SIZE             = 64;
	final public static int DISK_STRUCT_BM_NAME_SIZE                = 512;

	final public static int DISK_STRUCT_N_MODULES                   = 30;

	// Numbers of blocks to look for superblock copies in - usually first 3 are used
	final public static long DISK_STRUCT_SB_OFFSET_LIST[] = { 0x10, 0x100, 0x220, 0x333 };
	
	// Superblock's fields offsets
	final public static int magic 				= 0;
	final public static int version 				= 4;
	final public static int checksum				= 8;
	final public static int blocksize				= 12;
	final public static int sb2_addr				= 16;
	final public static int sb3_addr				= 20;
	final public static int disk_start_page		= 24;
	final public static int disk_page_count		= 28;
	final public static int free_start			= 32;
	final public static int free_list				= 36;
	final public static int fs_is_clean			= 40;
	final public static int general_flags_1		= 41;
	final public static int general_flags_2		= 42;
	final public static int general_flags_3		= 43;
	final public static int last_snap				= 44;
	final public static int last_snap_time		= 48;
	final public static int last_snap_crc32		= 52;
	final public static int prev_snap				= 56;
	final public static int prev_snap_time		= 60;
	final public static int prev_snap_crc32		= 64;
	final public static int magic2				= 68;
	final public static int boot_list				= 72;
	final public static int kernel_list			= 76;
	final public static int boot_progress_page	= 80;
	final public static int boot_module			= 84;
	final public static int last_boot_time		= 204;
	final public static int bad_list				= 208;
	final public static int log_list				= 212;
	final public static int sys_name				= 216;
	final public static int object_space_address	= 280;
	final public static int last_short_journal_blocks 	= 284;
	final public static int last_long_journal_root		= 540;
	final public static int last_short_journal_flags		= 544;
	final public static int last_long_journal_flags		= 545;
	final public static int prev_short_journal_blocks		= 548;
	final public static int prev_long_journal_root		= 804;
	final public static int prev_short_journal_flags		= 808;
	final public static int prev_long_journal_flags		= 809;

}
