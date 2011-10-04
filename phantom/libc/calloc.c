#include <malloc.h>
#include <string.h>

void *
calloc(size_t number, size_t size)
{
    void *ret;

    ret = malloc(number * size);
    if(ret != 0)
        memset(ret, 0, number * size);

    return ret;
}

