/**
 *
 * Phantom OS
 *
 * Phantom 'FS' extraction code - to debug snapshotting.
 *
 * (C) 2005-2008 dz.
 *
**/

#include "fsio.h"

static fsio io;
char sbbuf[DISK_STRUCT_BS];
phantom_disk_superblock *sb = (phantom_disk_superblock *)&sbbuf;

void storeToFile( long startBlk, char *outFn, int magic );

long sbaddr[] = DISK_STRUCT_SB_OFFSET_LIST;


int extract(const char *fn)
{

    io.open( fn );

    int rsb_success = 0;

    for( int i = 0; i < 4; i++ )
    {
        io.read( sbbuf, sbaddr[i] );

        if(
           sb->magic == DISK_STRUCT_MAGIC_SUPERBLOCK &&
           sb->magic2 == DISK_STRUCT_MAGIC_SUPER_2 )
        {
            rsb_success = 1;
            break;
        }
    }

    if(!rsb_success)
    {

        printf("Wrong sb magics\n");
        return 33;
    }

    printf("FS Ver. %d.%d, osname '%.*s'\n", sb->version >> 16, sb->version & 0xFFFF,
           DISK_STRUCT_SB_SYSNAME_SIZE, sb->sys_name );
    printf("Free list at %d\n", sb->free_list);

    printf("Disk is %d blockls, untouched space from %d\n", sb->disk_page_count, sb->free_start);


    storeToFile( sb->last_snap, "last_snap.data", DISK_STRUCT_MAGIC_SNAP_LIST );
    storeToFile( sb->prev_snap, "prev_snap.data", DISK_STRUCT_MAGIC_SNAP_LIST );

    storeToFile( sb->boot_list, "boot.data", DISK_STRUCT_MAGIC_BOOT_LOADER );
    storeToFile( sb->kernel_list, "kernel.data", DISK_STRUCT_MAGIC_BOOT_KERNEL );

    storeToFile( sb->boot_module[0], "boot_module.0.data", DISK_STRUCT_MAGIC_BOOT_MODULE );
    storeToFile( sb->boot_module[1], "boot_module.1.data", DISK_STRUCT_MAGIC_BOOT_MODULE );
    storeToFile( sb->boot_module[2], "boot_module.2.data", DISK_STRUCT_MAGIC_BOOT_MODULE );

    io.close();
}


void storeToFile( long startBlk, char *outFn, int magic )
{
    FILE *out = fopen(outFn, "wb");
    setvbuf( out, NULL, _IOFBF, 1024*1024 );
    if( out == NULL )
    {
        printf("Can't open %s\n", outFn );
        return;
    }

    fslist fl( startBlk, &io, magic );

    char data[DISK_STRUCT_BS];
    int len;

    while( (len = fl.read(data, DISK_STRUCT_BS)) > 0 )
    {
        if( len != fwrite( data, 1, len, out) )
        {
            printf("%s write error", outFn);
            break;
        }
        //printf("%d ", len);
    }

    fclose( out );
}



int main(int ac, char **av)
{
    if( ac != 2 ) return 33;

    setvbuf( stdout, NULL, _IOLBF, 128 );

    try { return extract(av[1]); }
    catch(fsioError e)
    {
        printf("Exception: %s", e.getMessage() );
    }

}
