#include <user/sys_fio.h>
#include <string.h>
#include <stdio.h>

#include "stdio_private.h"

static int _write(void* arg, const void* buf, ssize_t len)
{
    int err;
    if(((FILE*)arg)->buf_pos > 0)
    {
        err = write(((FILE*)arg)->fd, ((FILE*)arg)->buf, ((FILE*)arg)->buf_pos);
        if(err < 0)
        {
            errno = EIO;
            ((FILE*)arg)->flags |= _STDIO_ERROR;
            return err;
        }
        ((FILE*)arg)->buf_pos = 0;
    }
    err = write(((FILE*)arg)->fd, buf, len);
    if(err < 0)
    {
        errno = EIO;
        ((FILE*)arg)->flags |= _STDIO_ERROR;
    }
    return err;
}

int vfprintf(FILE *stream, char const *format, va_list ap)
{
    return _v_printf(_write, stream, format, ap);
}
