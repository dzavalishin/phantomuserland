/**
 *
 * Phantom OS
 *
 * Phantom 'FS' format code - to prepare image files, mostly.
 *
 * (C) 2005-2008 dz.
 *
**/

#include <stdint.h>

// or else it conflicts with compiler's types.h
#define _TIME_T_DECLARED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <phantom_disk.h>


long sbaddr[] = DISK_STRUCT_SB_OFFSET_LIST;

#define CHECK(code, text) do { if(code) { printf(text); exit(33); } } while(0)
#define CHECKA(code, text, arg) do { if(code) { printf(text,arg); exit(33); } } while(0)

int 
main( int ac, char **av )
{
	int disk_size = 1024*20;

	if( ac != 2 ) 
		{
		printf("Phantom disk formatter.\n\nusage: %s phantom_disk_file_name\n",
			av[0] );
		exit(1);
		}

	FILE *fp = fopen( av[1], "r+b" );
        if(fp == NULL ) fp = fopen( av[1], "w+b" );

	if( fp == 0 )
		{
		printf("Can't create %s\n", av[1] );
		exit(1);
		}


	union {
		phantom_disk_superblock sb;
		char _nothing[DISK_STRUCT_BS];
	} u;

	memset( &u, 0, sizeof(u) );

	phantom_disk_format( &u.sb, disk_size, "Unnamed Phantom system" );

        printf("Disk size is %ld pages\n", (unsigned long)u.sb.disk_page_count);
        printf("Disk block size is %ld\n", (long)DISK_STRUCT_BS );

#if 0
        // This is for older kernel - TODO - REMOVE
	if( 1 != fwrite( &u, DISK_STRUCT_BS, 1, fp ) )
		{
		printf("Can't write to %s\n", av[1] );
		exit(1);
		}
#endif

        long sba = DISK_STRUCT_BS*(long)sbaddr[0];
        printf("Writing superblock to 0x%X\n", sba);

        CHECKA(fseek( fp, sba, SEEK_SET ),"can't seek to 0x%lX\n", sba );

        // This is at 0x10 - for actual kernel
	if( 1 != fwrite( &u, DISK_STRUCT_BS, 1, fp ) )
		{
		printf("Can't write to %s\n", av[1] );
		exit(1);
		}

        // Now make sure we can write there or, for file
        // image of fs, extend file size

        // TODO it does not work - why????!

        sba = DISK_STRUCT_BS*((long)(u.sb.disk_page_count-1));
        printf("Writing superblock to 0x%X\n", sba);

        CHECK(fseek( fp, sba, SEEK_SET ),"can't seek\n");

	CHECKA( 1 != fwrite( &u, DISK_STRUCT_BS, 1, fp ), "Can't write to %s\n", av[1] );

        fclose(fp);
	return 0;
}
