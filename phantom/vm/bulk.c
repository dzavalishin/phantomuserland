/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 *
 * Bulk class saver/loader. The idea is to prepare a huge list of stored classes for
 * a new OS to init itself from. List is to be stored on a CD or on a new system fresh
 * formatted disk in a boot module.
 *
 **/


#define DEBUG_MSG_PREFIX "vm.bulk"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>
#include <assert.h>


#include "vm/internal_da.h"
#include "vm/code.h"
#include "win_bulk.h"
#include "main.h"

#define DEBUG 0


static pvm_bulk_read_t  readf;
static pvm_bulk_seek_t  seekf;


void pvm_bulk_init( pvm_bulk_seek_t sf, pvm_bulk_read_t rd )
{
    readf = rd;
    seekf = sf;
}

static int skip( int len );
static int load( int len, struct pvm_object   *out );
static int cncmp( const char *a, const char *b );



/**
 *
 * \brief Find class object by class name.
 *
 * \param[in]  class_name        Name of class to find.
 * \param[out] out               Class object found.
 *
 * \return 0 on success, nonzero if not found..
 *
 * Called from root.c to load boot class on object land creation
 * and from syscall (internal boot class) to be used by userland loader
 * if other methods fail.
 *
**/


// Return 0 on success
int pvm_load_class_from_module( const char *class_name, struct pvm_object   *out )
{
    seekf(0);

    if(DEBUG) printf("Bulk: looking for class %s\n", class_name);

    // If we are in user mode (pvm_test/pvm_headless), or kernel
    // has some disks mounted, try to find class in .pc file.
    if( 0 == load_class_from_file( class_name, out) )
        return 0;

    while(1)
    {
        int rlen;
        struct pvm_bulk_class_head      ch;

        rlen = readf( sizeof( struct pvm_bulk_class_head ), &ch );
        if( rlen != sizeof( struct pvm_bulk_class_head ) )
        {
            // TODO print err
            break;
        }

        if(DEBUG) printf("Bulk: checking class %s\n", ch.name);

        if( 0 == cncmp( class_name, ch.name ) )
            return load(ch.data_length, out);
        else
            skip(ch.data_length);
    }

    return -1;
}


static int skip( int len )
{
    char buf[512];

    while( len > 0 )
    {
        int l = len;
        if( l > 512 ) l = 512;

        int ret = readf( l, buf );
        if( ret <= 0 )
            return -1;

        len -= ret;
    }
    return 0;
}


static int load( int len, struct pvm_object   *out )
{
    if(DEBUG) printf("Bulk: loading\n" );

    void *buf = malloc(len);
    if( buf == 0 )
        return -1;


    int rret = readf( len, buf );
    if( rret != len )
        return -1;


    int lret = pvm_load_class_from_memory( buf, len, out );
    free( buf );

    return lret;
}

__inline__ static int issepa( char c )
{
    return c == '.' || c == '/';
}

static int cncmp( const char *a, const char *b )
{
    if( issepa( *a ) ) a++;
    if( issepa( *b ) ) b++;


    for( ; *a && *b; a++, b++ )
    {
        if( *a == *b )
            continue;
        if( issepa( *a ) && issepa( *b ) )
            continue;
        return -1;
    }

    return !( (*a == 0) && (*b == 0) ); // both are zero? return 0
}





