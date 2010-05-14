//#include <stdio.h>

void
testPrint( int errno, const char *name )
{
    int success = !errno;
    printf("%s: %s\n", name, success ? "success" : "ERROR!");
    if(!success)
    {
        char buf[1024];
        if( strerror_r( errno, buf, 1024) )
            printf("Can't tell err! %d\n", errno);
        else
            printf("Errno = %d, %s\n", errno, buf );
        sleep(1);
    }
}
