#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "arlib.h"

const char *fname = "libphantom_vm.a";

FILE *fp;

int rf( void *data, int len )
{
    return fread( data, 1, len, fp );
}

void pf(const char *fname, void *data, int len)
{
    printf("'%s', size %d\n", fname, len);
}

int main( int ac, char ** av )
{
    fp = fopen( fname, "rb" );
    if(fp == 0)
    {
        printf( "can't open %s\n", fname );
        return 33;
    }

    arread( rf, pf );

    return 0;
}

