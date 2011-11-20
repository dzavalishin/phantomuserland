#include <user/sys_fio.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "stdio_private.h"

static long _ftell(FILE* stream);




long ftell(FILE* stream)
{
    fpos_t p;
#if STDIO_SEM
    _kern_sem_acquire(stream->sid, 1);
    p = _ftell(stream);
    _kern_sem_release(stream->sid, 1);
#else // STDIO_SEM
    p = _ftell(stream);
#endif // STDIO_SEM

    return p;
}

static long _ftell(FILE* stream)
{
    fpos_t p;

    _flush(stream);

    p = lseek(stream->fd, 0, SEEK_CUR) - ((stream->flags & _STDIO_UNGET) ? 1 : 0);

    if(p < 0)
    {
        errno = EIO;
    }
    return p;
}

int fseek(FILE *stream, long int offset, int whence)
{
    fpos_t p;

    _flush(stream);

    p = lseek(stream->fd, offset, whence);

    if(p < 0)
    {
        errno = EIO;
    }
    return p;
}

int fgetpos(FILE *stream, fpos_t *pos)
{
    fpos_t p = ftell(stream);
    if(p < 0)
    {
        return p;
    }
    *pos = p;
    return 0;
}


int fflush(FILE *stream)
{
    if(stream == (FILE*)0)
    {
        FILE* node = __open_file_stack_top;
        int err = 0;
        while(node != (FILE*)0)
        {
            if(fflush(node) == EOF)
                err = EOF;
            node = node->next;
        }
        return err;
    }
    else
    {
        int err;
#if STDIO_SEM
        _kern_sem_acquire(stream->sid, 1);
        err = _flush(stream);
        _kern_sem_release(stream->sid, 1);
#else // STDIO_SEM
        err = _flush(stream);
#endif // STDIO_SEM
        return err;
    }
}

int _flush(FILE* stream)
{
    if(stream->buf_pos)
    {
        if(stream->flags & _STDIO_WRITE)
        {
            int err = write(stream->fd, stream->buf, stream->buf_pos);
            stream->buf_pos = 0;
            if(err < 0)
            {
                errno = EIO;
                stream->flags |= _STDIO_ERROR;
                return EOF;
            }

        }
        else if(stream->flags & _STDIO_READ)
        {
            off_t dif = stream->rpos - stream->buf_pos;
            if(dif < 0)
            {
                dif = lseek(stream->fd, dif, SEEK_CUR);
                stream->rpos = stream->buf_pos = 0;
                if(dif < 0)
                {
                    errno = EIO;
                    stream->flags |= _STDIO_ERROR;
                    return EOF;
                }
            }
        }
    }
    return 0;

}

#if STDIO_SEM

int feof(FILE *stream)
{
    int i = 0;
    _kern_sem_acquire(stream->sid, 1);
    i = stream->flags & _STDIO_EOF;
    _kern_sem_release(stream->sid, 1);
    return i;
}

int ferror (FILE *stream)
{
    int i = 0;
    _kern_sem_acquire(stream->sid, 1);
    i = stream->flags & _STDIO_ERROR;
    _kern_sem_release(stream->sid, 1);
    return i;
}

void clearerr(FILE *stream)
{
    _kern_sem_acquire(stream->sid, 1);
    stream->flags &= ~_STDIO_ERROR;
    _kern_sem_release(stream->sid, 1);
}

#else // STDIO_SEM

int feof(FILE *stream)
{
    return stream->flags & _STDIO_EOF;
}

int ferror (FILE *stream)
{
    return stream->flags & _STDIO_ERROR;
}

void clearerr(FILE *stream)
{
    stream->flags &= ~_STDIO_ERROR;
}


#endif // STDIO_SEM


int fileno(FILE *stream)
{
    return stream->fd;
}
