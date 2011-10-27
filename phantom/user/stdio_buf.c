#include <user/sys_fio.h>
#include <string.h>
#include <stdio.h>

#include "stdio_private.h"

#if !STDIO_SEM
#  define _kern_sem_acquire(a,b)
#  define _kern_sem_release(a,b)
#endif


int setvbuf(FILE *stream, char *buf, int mode, size_t size)
{
    _kern_sem_acquire(stream->sid, 1);

    _flush(stream);

    if(stream->buf)
        free(stream->buf);

    if( (mode != _IOFBF) && (mode != _IOLBF) )
        mode = _IONBF;

    if( (buf == 0) || (size == 0) )
    {
        mode = _IONBF;
        size = 0;
    }

    stream->buf = (unsigned char *)buf;
    stream->buf_size = size;
    stream->buf_mode = mode;

    _kern_sem_release(stream->sid, 1);

    return 0;
}

void setbuf(FILE *stream, char *buf)
{
    setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}

void setbuffer(FILE *stream, char *buf, int size)
{
    setvbuf(stream, buf, buf ? _IOFBF : _IONBF, (size_t)size);
}

int setlinebuf(FILE *stream)
{
    return setvbuf(stream, NULL, _IOLBF, (size_t)0);
}
