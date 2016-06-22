
#include "cpfs.h"
#include "cpfs_local.h"

#include <stdio.h>
#include <stdlib.h>


void die_rc( const char *msg, int rc )
{
    printf("%s, rc = %d\n", msg, rc );
    exit( 0 );
}


void test(void)
{
}



int main( int ac, char**av )
{

    errno_t 		rc;

    if(sizeof(uint64_t) < 8)
        die_rc( "int64", sizeof(uint64_t) );



    rc = cpfs_init();
    if( rc ) die_rc( "Init", rc );


    test();

    rc = cpfs_stop();
    if( rc ) die_rc( "Stop", rc );


    return 0;
}


void
cpfs_panic( const char *msg )
{
    printf( "Panic: %s\n", msg );
    exit(33);
}

