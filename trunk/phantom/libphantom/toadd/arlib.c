#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "arlib.h"


#include <endian.h>
#include <asm/byteorder.h>


#define GHDR "!<arch>\n"
#define GHSIZE 8

#define FHSIZE 60

struct arreader
{

};


static void *loadall( struct arreader *a, int (*rf)( void *data, int len ), int size );
static void parseIndex(struct arreader *a, void *arIndex, int size);
//static void rdata( int (*rf)( void *data, int len ), int size, const char *name );

static void die( struct arreader *a, const char*m);
static char * getLongName( struct arreader *a, char *nametab, int pos );
static void pack(unsigned char *s);



int arread( int (*rf)( void *data, int len ), void (*process_f)(const char *fname, void *data, int len) )
{
    char *nametab = 0;
    char *arIndex = 0;

    char ghbuf[GHSIZE];



    struct arreader a;




    if( GHSIZE != rf( ghbuf, GHSIZE ))
    {
        die( &a, "no hdr");
    }


    while(1)
    {
        char fhbuf[FHSIZE];

        int ret = rf( fhbuf, FHSIZE );

        if( ret == 0 )
            break;

        if( FHSIZE != ret )
        {
            die( &a, "no fhdr");
        }

        char r_fname[17];
        strlcpy( r_fname, fhbuf, 16 );

        char r_fsize[10];
        strlcpy( r_fsize, fhbuf+48, 9 );


        if( fhbuf[58] != 0x60u || fhbuf[59] != 0x0Au )
        {
            //hexdump(fhbuf, FHSIZE, "", 0);
            printf("name '%s', size '%s' magic: %x %x\n", r_fname, r_fsize, fhbuf[58], fhbuf[59] );
            die( &a, "no file magic" );
        }


        //printf("name '%s', size '%s'\n", r_fname, r_fsize );

        //printf("name '%s'\n", r_fname );

        int size = (int)atol(r_fsize);

        pack((unsigned char *)r_fname);
        //printf("name '%s'\n", r_fname );


        int isSlash = !strcmp( r_fname, "/" );
        if(isSlash)
        {
            /*
            char *arIndex = calloc( size+1, 1 );
            if( arIndex == 0 || size != rf( arIndex, size ) )
                die( &a, "can't load /");
                */

            arIndex = loadall( &a, rf, size );
            //printf("/=\n'%.*s'\n", size, arIndex );
            parseIndex( &a, arIndex, size);
        }


        int isSlashSlash = !strcmp( r_fname, "//" );

        if(isSlashSlash)
        {
            /*
            nametab = calloc( size+1, 1 );
            if( nametab == 0 || size != rf( nametab, size ) )
                die( &a, "can't load //");
            */

            nametab = loadall( &a, rf, size );


            //printf("//=\n%.*s\n", size, nametab );
        }

        char *fname = r_fname;

        if( r_fname[0] == '/' && r_fname[1] )
            fname = getLongName( &a, nametab, (int) atol(r_fname+1) );


        if(!isSlashSlash && !isSlash)
        {
            //rdata( rf, size, fname );
            void *d = loadall( &a, rf, size );
            process_f( fname, d, size );
            free(d);
        }

        // align at 2 bytes
        if( size & 1 )
            rf( fhbuf, 1 );

    }


    if(nametab) free(nametab);
    if(arIndex) free(arIndex);

    return 0;

    /*
die:
    if(nametab) free(nametab);
    if(arIndex) free(arIndex);
    return -1;
    */
}





void *loadall(struct arreader *a, int (*rf)( void *data, int len ), int size )
{
    char *ret = calloc( size+1, 1 );
    if( ret == 0 || size != rf( ret, size ) )
        die( a, "can't loadall");

    //printf("/=\n'%.*s'\n", size, ret );
    return ret;
}



void parseIndex(struct arreader *a, void *arIndex, int size)
{
    int num = ntohl( *((u_int32_t*)arIndex) );

    //printf("index size=%d\n", num );

    int *positions = arIndex + sizeof(u_int32_t);

    char *names = arIndex + (sizeof(u_int32_t) * (num+1));

    int nstring;
    for( nstring = 0; nstring < num; nstring++ )
    {
        char *name = names;

        while( *names )
        {
            names++;
            if( (void *)names >= arIndex+size )
                die( a, "ar index is broken");
        }

        // skip 0
        names++;

        //printf("'%s' @ %d\t", name, ntohl(positions[nstring]) );
    }
    //printf("\n\n");

}




/*
#define BUFSZ   4096
void rdata( int (*rf)( void *data, int len ), int size, const char *name )
{
    char buf[BUFSZ];

    printf("read data name '%s', size %d\n", name, size );


    while( size > 0 )
    {
        int toread = size;
        if( toread > BUFSZ ) toread = BUFSZ;

        int rc = rf( buf, toread );

        if( rc <= 0 )
            die( &a, "read fail");


        // get rc bytes

        size -= rc;
    }

}
*/


static void die( struct arreader *a, const char*m)
{
    printf("%s\n",m);
    exit(22);
}


static void pack(unsigned char *s)
{
    int pos = strlen((char *)s);

    for( ; --pos >= 0; )
    {
        // end of name marker
        if( s[pos] == '/' )
        {
            // save / and // names
            if( pos < 2 && s[0] == '/' )
                break;

            s[pos] = 0;
            break;
        }


        //if( s[pos] <= ' ' )
        if( s[pos] == ' ' )
            s[pos] = 0;
        else
            break;
    }
}





static char * getLongName( struct arreader *a, char *nametab, int pos )
{
    if( nametab == 0 )
        die( a, "No nametab");

    char *tp = nametab+pos;

    while( *tp )
    {
        tp++;
        if( *tp == '\n' )
            *tp = 0;
    }

    return nametab+pos;
}











