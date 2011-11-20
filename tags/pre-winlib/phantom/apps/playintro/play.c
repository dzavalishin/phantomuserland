#include <stdio.h>
//#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/fcntl.h>
#include <sys/types.h>


#define DEFAULT_DSP	"/dev/pci/es1370.0"


unsigned char samples[] =
{
#include "intro.ci"
};


int main( int argc, char **argv )
{
    (void) argc;
    (void) argv;

    int fd;

    if ((fd = open( DEFAULT_DSP, O_WRONLY)) < 0)
    {
        printf("Can't open DSP '%s'\n", DEFAULT_DSP );
        return 1;
    }

    write( fd, samples, sizeof(samples));
    close( fd );

    return 0;
}

