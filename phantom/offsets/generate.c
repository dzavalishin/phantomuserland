#include <sys/cdefs.h>
#include "offsets.h"

#define O(t,f)	out_offset(#t,#f, __builtin_offsetof(t,f) )
//#define S(t,f)	out_size(#t,#f, sizeof(t,f) )


#include <phantom_disk.h>
#include <phantom_libc.h>






void generate()
{
	O(phantom_disk_superblock,magic);
	O(phantom_disk_superblock,version);
	O(phantom_disk_superblock,checksum);
	O(phantom_disk_superblock,blocksize);
	O(phantom_disk_superblock,sb2_addr);
	O(phantom_disk_superblock,sb3_addr);
	O(phantom_disk_superblock,disk_start_page);
	O(phantom_disk_superblock,disk_page_count);
	O(phantom_disk_superblock,free_start);
	O(phantom_disk_superblock,free_list);
	O(phantom_disk_superblock,fs_is_clean);
	O(phantom_disk_superblock,last_snap);
	O(phantom_disk_superblock,last_snap_time);
	O(phantom_disk_superblock,last_snap_crc32);
	O(phantom_disk_superblock,prev_snap);
	O(phantom_disk_superblock,prev_snap_time);
	O(phantom_disk_superblock,prev_snap_crc32);
	O(phantom_disk_superblock,magic2);
	O(phantom_disk_superblock,boot_list);
	O(phantom_disk_superblock,kernel_list);
	O(phantom_disk_superblock,boot_progress_page);
	O(phantom_disk_superblock,boot_module);
	O(phantom_disk_superblock,last_boot_time);
	O(phantom_disk_superblock,bad_list);
	O(phantom_disk_superblock,log_list);
	O(phantom_disk_superblock,sys_name);
	O(phantom_disk_superblock,object_space_address);
	O(phantom_disk_superblock,last_short_journal_blocks);
	O(phantom_disk_superblock,last_long_journal_root);
	O(phantom_disk_superblock,last_short_journal_flags);
	O(phantom_disk_superblock,last_long_journal_flags);
	O(phantom_disk_superblock,prev_short_journal_blocks);
	O(phantom_disk_superblock,prev_long_journal_root);
	O(phantom_disk_superblock,prev_short_journal_flags);
	O(phantom_disk_superblock,prev_long_journal_flags);
}


