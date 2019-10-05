#include <phantom_libc.h>

#include <stdlib.h>



long
atoln(const char *str, size_t n)
{
    char buf[80];

    if( n > (sizeof(buf) - 1) )
        n = sizeof(buf) - 1;

    strlcpy( buf, str, n );

    return(strtol(buf, (char **)0, 10));
}

