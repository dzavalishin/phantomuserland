#include <string.h>
#include <phantom_types.h>
#include <phantom_libc.h>

char * strlwr(char *in)
{
    char *ret = in;

    for( ; *in; in++ )
    {
        if(isupper(*in))
            *in = tolower(*in);
    }

    return ret;
}


