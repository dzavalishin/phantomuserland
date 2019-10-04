/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Filesystem map. Looks up disk partition and finds suitable handler.
 *
**/


#define DEBUG_MSG_PREFIX "fs_map"
#include "debug_ext.h"
#define debug_level_flow 1
#define debug_level_error 10
#define debug_level_info 1

#include <phantom_libc.h>
#include <string.h>
#include <phantom_disk.h>
#include <kernel/page.h>

#include "fs_map.h"




typedef struct
{
    const char *name;
    errno_t (*probe_f)( phantom_disk_partition_t * );        	// FS probe func
    errno_t (*use_f)( phantom_disk_partition_t * );        	// FS activate func
} fs_probe_t;





static fs_probe_t fs_drivers[] =
{

    { "Phantom", 	fs_probe_phantom,	fs_use_phantom              },

#if HAVE_UNIX
    { "FAT", 		fs_probe_fat, 	 	fs_start_fat                },
//    { "Ext2",  		fs_probe_ext2,      fs_start_ext2               },
    { "CD",  		fs_probe_cd, 	 	fs_start_cd                 },
#endif // HAVE_UNIX

};

#define FS_START_THREAD 1

#if FS_START_THREAD
#include <threads.h>
#endif

errno_t lookup_fs(phantom_disk_partition_t *p)
{
    char pname[128];
    partGetName( p, pname, sizeof(pname) );

    SHOW_INFO( 2, "Look for filesystems on partition %s", pname );
    unsigned int i;
    for( i = 0; i < sizeof(fs_drivers)/sizeof(fs_probe_t); i++ )
    {
        fs_probe_t *fp = &fs_drivers[i];

        SHOW_INFO( 3, "probe %s fs on %s", fp->name, pname );

        errno_t ret = fp->probe_f( p );
        if( ret ) continue;

        SHOW_INFO( 2, "%s file sysem found on partition %s", fp->name, pname );

        if(!fp->use_f)
        {
            SHOW_ERROR( 0, "%s file sysem is not implemented yet", fp->name );
            continue;
        }

#if FS_START_THREAD
        // BUG HACK - activate phantom fs syncronously, or else we will die attempting to use it
        if(fp->use_f == fs_use_phantom)
            fp->use_f( p );
        else
            hal_start_kernel_thread_arg( (void (*)(void *))fp->use_f, p );
#else
        ret = fp->use_f( p );
        if( ret )
        {
            SHOW_ERROR( 0, "%s file sysem driver rejected partition %s", fp->name, pname );
            continue;
        }

        SHOW_INFO( 1, "%s file sysem driver took partition %s", fp->name, pname );
#endif
        return 0;
    }

    return EINVAL;
}




static disk_page_no_t sbpos[] = DISK_STRUCT_SB_OFFSET_LIST;
static int nsbpos = sizeof(sbpos)/sizeof(disk_page_no_t);

#define MAX_PFS_PARTS 10
static phantom_disk_partition_t *phantom_fs_partitions[MAX_PFS_PARTS];
static int n_phantom_fs_partitions = 0;

errno_t fs_probe_phantom(phantom_disk_partition_t *p)
{
    char buf[PAGE_SIZE];
    phantom_disk_superblock *sb = (phantom_disk_superblock *)&buf;

    int i;
    for( i = 0; i < nsbpos; i++ )
    {
        SHOW_INFO( 9, "part %s read phantom sb @%d", p->name, sbpos[i] );
        if( phantom_sync_read_block( p, buf, sbpos[i], 1 ) )
            continue;
        SHOW_INFO0( 9, "calc phantom sb checksum" );
        if( phantom_calc_sb_checksum( sb ) )
        {
            SHOW_INFO0( 9, "phantom sb checksum ok" );
            p->flags |= PART_FLAG_IS_PHANTOM_FSSB;
            return 0;
        }

        //hexdump(const void *ptr, int length, const char *hdr, int flags)
        //phantom_dump_superblock(sb);

    }

    return EINVAL;
}

errno_t fs_use_phantom(phantom_disk_partition_t *p)
{
    assert( p->flags & PART_FLAG_IS_PHANTOM_FSSB );

    char pname[128];
    partGetName( p, pname, sizeof(pname) );

    if(n_phantom_fs_partitions < MAX_PFS_PARTS)
    {
        phantom_fs_partitions[n_phantom_fs_partitions++] = p;
        return 0;
    }
    else
    {
        SHOW_ERROR( 0, "Too many Phantom disks, skip %s", pname);
        return EMFILE;
    }
}


phantom_disk_partition_t *select_phantom_partition(void)
{
    if(n_phantom_fs_partitions == 0)
        panic("no Phantom disk");

    if(n_phantom_fs_partitions == 1)
    {
        assert( phantom_fs_partitions[0] );

        char pname[128];
        partGetName( phantom_fs_partitions[0], pname, sizeof(pname) );

        SHOW_INFO( 1, "Just one Phantom disks found (%s)", pname);
        return phantom_fs_partitions[0];
    }

    // TODO auto-select by last snap date
    printf("Select Phantom disk to boot:\n");
    int i;
    for( i = 0; i < n_phantom_fs_partitions; i++ )
    {
        phantom_disk_partition_t *p = phantom_fs_partitions[i];
        if(p)
        {
            char pname[128];
            partGetName( p, pname, sizeof(pname) );

            printf("%2d: %s\n", i, pname );
        }
    }

    do {

        printf("Press digit (0-%d): \n", n_phantom_fs_partitions-1 );
        char c = (char)getchar();

        if( c < '0' || c > '9' )
            continue;

        int n = c-'0';
#if MAX_PFS_PARTS < 10
        if( (n > MAX_PFS_PARTS) || (n < 0) )
            continue;
#endif
        phantom_disk_partition_t *p = phantom_fs_partitions[n];

        if( !p )
            continue;

        return p;

    } while(1);
}










