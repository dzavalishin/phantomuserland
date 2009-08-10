/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: no
 * Preliminary: yes
 *
 * This source file implements Windows based wrapper for VM to
 * run in Windows-hosted environment.
 *
**/

#include <stdlib.h>
#include "win_bulk.h"


#include "gcc_replacements.h"

/**
 *
 * Load file to memory - for tests
 *
 **/


int load_code(void **out_code, unsigned int *out_size, const char *fn)
{
    FILE * f = fopen( fn, "rb" );

    if( f == NULL )
    {
        //if(debug_print) printf("Can't open %s\n", fn );
        return 1;
    }

    fseek( f, 0, SEEK_END );
    long fsize = ftell(f);
    //printf("fsize %d\n", fsize );

    unsigned char *code = (unsigned char *)malloc(fsize);
    if( code == NULL )
    {
        printf("Can't alloc mem\n" );
        return 1;
    }

    fseek( f, 0, SEEK_SET );
    int ret = fread( code, 1, fsize, f );
    if( fsize != ret )
    {
        printf("Can't read code: ret = %d\n", ret );
        free( code );
        return 1;
    }

    fclose( f );

    *out_size = (unsigned)fsize;
    *out_code = code;

    return 0;
}


// -----------------------------------------------------------------------
// bulk

void *bulk_code;
unsigned int bulk_size;
void *bulk_read_pos;

int bulk_seek_f( int pos )
{
    bulk_read_pos = bulk_code + pos;
    return bulk_read_pos >= bulk_code + bulk_size;
}

int bulk_read_f( int count, void *data )
{
    if( count < 0 ) return -1;

    int left = (bulk_code + bulk_size) - bulk_read_pos;

    if( count > left ) count = left;

    memcpy( data, bulk_read_pos, count );

    bulk_read_pos += count;

    return count;
}


void save_mem( void *mem, int size )
{
    printf("Creating mem dump file\n" );
    FILE * f = fopen( "wsnap.dump", "wb" );

    if( f == NULL )
    {
        printf("Can't create mem dump file\n" );
    }

    fseek( f, 0, SEEK_END );
    if( 1 != fwrite( mem, size, 1, f ) )
        printf("Can't save mem dump, IO error\n" );

}

