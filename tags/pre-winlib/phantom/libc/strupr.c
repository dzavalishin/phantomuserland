#include <string.h>
#include <phantom_types.h>
#include <phantom_libc.h>

char * strupr(char *in)
{
    char *ret = in;

    for( ; *in; in++ )
    {
        if(islower(*in))
            *in = toupper(*in);
    }

    return ret;
}


