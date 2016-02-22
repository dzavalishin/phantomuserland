#include <user/sys_fio.h>
#include <string.h>
#include <stdio.h>

#include "stdio_private.h"

static int _fputc(int ch, FILE *stream);
static int _fgetc(FILE* stream);


#if !STDIO_SEM
#  define _kern_sem_acquire(a,b)
#  define _kern_sem_release(a,b)
#endif


int putchar(int c) { return fputc( c, stdout ); }

int puts(const char *str)
{
    int rc = fputs(str, stdout);
    fputs("\n", stdout); // TODO fixme retcode ign
    return rc;
}



int ungetc(int c, FILE *stream)
{
    _kern_sem_acquire(stream->sid, 1);
    if(stream->flags & _STDIO_UNGET)
    {
        _kern_sem_release(stream->sid, 1);
        return EOF;
    }
    stream->flags &= ~_STDIO_EOF;
    stream->flags |= _STDIO_UNGET;
    stream->unget = c;
    _kern_sem_release(stream->sid, 1);
    return c;
}

int putc(int ch, FILE *stream)
{
    return fputc(ch, stream);
}

int fputc(int ch, FILE *stream)
{
    int ret_ch;
    _kern_sem_acquire(stream->sid, 1);
    ret_ch = _fputc(ch, stream);
    _kern_sem_release(stream->sid, 1);
    return ret_ch;
}

int fputs(const char *str, FILE *stream)
{
    _kern_sem_acquire(stream->sid, 1);
    while(*str != '\0')
    {
        int ret_val;
        if((ret_val = _fputc(*str++, stream)) < 0)
        {
            _kern_sem_release(stream->sid, 1);
            return ret_val;
        }
    }
    _kern_sem_release(stream->sid, 1);
    return 1;
}

static int _fputc(int ch, FILE *stream)
{
    if( stream->buf_mode == _IONBF )
    {
        char c = (char) ch;
        int err = write(stream->fd, &c, 1);
        if(err <= 0)
        {
            errno = EIO;
            stream->flags |= _STDIO_ERROR;
            return EOF;
        }
        return ch;
    }
    
    if(stream->buf_pos >= stream->buf_size)
    {
        int err = write(stream->fd, stream->buf, stream->buf_pos);
        if(err < 0)
        {
            errno = EIO;
            stream->flags |= _STDIO_ERROR;
            return EOF;
        }
        stream->buf_pos = 0;
    }

    (stream->buf[stream->buf_pos++] = (unsigned char)ch);

    if( ((ch == '\r') || (ch == '\n')) && (stream->buf_mode == _IOLBF) )
        fflush(stream);

    return ch;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    unsigned char* tmp = (unsigned char*)ptr;
    size_t i = nmemb;
    size_t j = size;

    _kern_sem_acquire(stream->sid, 1);

    for(;i > 0; i--)
    {
        for(; j > 0; j--)
        {
            int ch = _fputc(*tmp++, stream);

            if(ch < 0)
            {
                _kern_sem_release(stream->sid, 1);
                return nmemb - i;
            }
        }
        j = size;
    }
    _kern_sem_release(stream->sid, 1);

    return nmemb - i;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    unsigned char* tmp = (unsigned char*)ptr;
    size_t i = nmemb;
    size_t j = size;

    _kern_sem_acquire(stream->sid, 1);

    if (stream->flags & _STDIO_EOF)
    {
        return 0;
    }

    for(;i > 0; i--)
    {
        for(; j > 0; j--)
        {
            int c = _fgetc(stream);
            if(c < 0)
            {
                _kern_sem_release(stream->sid, 1);
                return nmemb - i;
            }

            *tmp++ = c;
        }
        j = size;
    }
    _kern_sem_release(stream->sid, 1);

    return nmemb - i;
}


char* fgets(char* str, int n, FILE * stream)
{
    unsigned char* tmp;
    int i = n-1;
    tmp = (unsigned char*)str;

    _kern_sem_acquire(stream->sid, 1);

    for(;i > 0; i--)
    {
        int c;

        if (stream->flags & _STDIO_EOF)
        {
            break;
        }

        c = _fgetc(stream);

        if(c < 0)
        {
            _kern_sem_release(stream->sid, 1);
            *tmp = '\0';
            return (char*)0;
        }
        *tmp++ = c;
        if(c == '\n')
            break;
    }

    _kern_sem_release(stream->sid, 1);

    *tmp = '\0';
    return str;
}

int getchar(void)
{
    return fgetc(stdin);
}

int getc(FILE *stream)
{
    return fgetc(stream);
}

int fgetc(FILE *stream)
{
    int c;
    _kern_sem_acquire(stream->sid, 1);
    c = _fgetc(stream);
    _kern_sem_release(stream->sid, 1);
    return c;
}

static int _fgetc(FILE* stream)
{
    int c;
    if(stream->flags & _STDIO_UNGET)
    {
        c = stream->unget;
        stream->flags &= stream->flags ^ _STDIO_UNGET;
    }
    else
    {
        if(stream->buf_mode == _IONBF)
        {
            if( stream == stdin ) // TODO doesnt work?
            {
                fflush(stdout);
                fflush(stderr);
            }

            char ch;
            int len = read(stream->fd, &ch, 1);

            if (len==0)
            {
                stream->flags |= _STDIO_EOF;
                return EOF;
            }
            else if (len < 0)
            {
                stream->flags |= _STDIO_ERROR;
                return EOF;
            }
            return ch;
        }

        if (stream->rpos >= stream->buf_pos)
        {
            if( stream == stdin ) // TODO doesnt work?
            {
                fflush(stdout);
                fflush(stderr);
            }

            int len = read(stream->fd, stream->buf, stream->buf_size);

            if (len==0)
            {
                stream->flags |= _STDIO_EOF;
                return EOF;
            }
            else if (len < 0)
            {
                stream->flags |= _STDIO_ERROR;
                return EOF;
            }
            stream->rpos=0;
            stream->buf_pos=len;
        }
        c = stream->buf[stream->rpos++];
    }
    return c;
}


int scanf(char const *fmt, ...)
{
    va_list args;
    int i;

    va_start(args, fmt);
    _kern_sem_acquire(stdin->sid, 1);
    i = vfscanf(stdin, fmt, args);
    _kern_sem_release(stdin->sid, 1);
    va_end(args);

    return i;
}

/*
int sscanf(char const *str, char const *fmt, ...)
{
    va_list args;
    int i;

    va_start(args, fmt);
    i = vsscanf(str, fmt, args);
    va_end(args);

    return i;
}
*/

int fscanf(FILE *stream, char const *fmt, ...)
{
    va_list args;
    int i;

    va_start(args, fmt);
    _kern_sem_acquire(stream->sid, 1);
    i = vfscanf(stream, fmt, args);
    _kern_sem_release(stream->sid, 1);
    va_end(args);

    return i;
}



int fprintf(FILE *stream, char const *fmt, ...)
{
    va_list args;
    int i;

    va_start(args, fmt);
    _kern_sem_acquire(stream->sid, 1);
    i = vfprintf(stream, fmt, args);
    _kern_sem_release(stream->sid, 1);
    va_end(args);

    return i;
}

