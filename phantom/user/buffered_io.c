#if 0
/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
** Justin Smith 2003/09/13
*/
#include <sys/syscalls.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <phantom_types.h>
#include <errno.h>

FILE *stdin;
FILE *stdout;
FILE *stderr;

int __stdio_init(void);	 /* keep the compiler happy, these two are not supposed */
int __stdio_deinit(void);/* to be called by anyone except crt0, and crt0 will change soon */

/* A stack of FILE's currently held by the user*/
FILE* __open_file_stack_top;
/* Semaphore used when adjusting the stack*/
sem_id __open_file_stack_sem_id;

static int _flush(FILE* stream);
static int _set_open_flags(const char* mode, int* sys_flags, int* flags);
static long _ftell(FILE* stream);
static int _flush(FILE* stream);
static int _fputc(int ch, FILE *stream);
static int _fgetc(FILE* stream);

static FILE *__create_FILE_struct(int fd, int flags)
{
    FILE *f;
    char name[32];

    /* Allocate the FILE*/
	f = (FILE *)malloc(sizeof(FILE));
	if(!f)
		return (FILE *)0;

    /* Allocate the buffer*/
    f->buf = (char *)malloc( BUFSIZ *sizeof(char));
    if(!f->buf)
    {
        free(f);
        return (FILE *)0;
    }

    /* Create a semaphore*/
    sprintf(name, "%d :FILE", fd);
    f->sid = _kern_sem_create(1, name);
    if(f->sid < 0)
    {
        free(f->buf);
        free(f);
        return (FILE *)0;
    }

    /* Fill in FILE values*/
    f->rpos = 0;
    f->buf_pos = 0;
    f->buf_size = BUFSIZ ;
	f->fd = fd;
    f->flags = flags;

    /* Setup list*/
    f->next = __open_file_stack_top;

    /* Put the FILE in the list*/
    _kern_sem_acquire(__open_file_stack_sem_id, 1);
    __open_file_stack_top = f;
    _kern_sem_release(__open_file_stack_sem_id, 1);
	return f;
}

static int __delete_FILE_struct(int fd)
{
    FILE *fNode, *fPrev;
	sem_id sid;
    /* Search for the FILE for the file descriptor */
    fPrev = (FILE*)0;
    fNode = __open_file_stack_top;
    while(fNode != (FILE*)0)
    {
        if(fNode->fd == fd)
        {
            break;
        }
        fPrev = fNode;
        fNode = fNode->next;
    }
    /* If it wasn't found return EOF*/
    if(fNode == (FILE*)0)
    {
        printf("Error: __delete_FILE_struct");
        return EOF;
    }

	/* Wait for the lock */
	sid = fNode->sid;
	_kern_sem_acquire(sid, 1);

	/* free the FILE space/semaphore*/
    _kern_close(fNode->fd);
    free(fNode->buf);
    free(fNode);

	/* Do we need to release before we delete? */
	_kern_sem_release(sid, 1);
    _kern_sem_delete(sid);

    /* Remove it from the list*/
    if(fNode == __open_file_stack_top)
    {
        _kern_sem_acquire(__open_file_stack_sem_id, 1);
        __open_file_stack_top = __open_file_stack_top->next;
        _kern_sem_release(__open_file_stack_sem_id, 1);
    }
    else
    {
        _kern_sem_acquire(__open_file_stack_sem_id, 1);
        fPrev->next = fNode->next;
        _kern_sem_release(__open_file_stack_sem_id, 1);
    }
    /* Free the space*/
    free(fNode);

    return 0;
}


int __stdio_init(void)
{
    /* Create semaphore*/
    __open_file_stack_sem_id = _kern_sem_create(1, "__open_file_stack");
    /*initialize stack*/
    __open_file_stack_top = (FILE*)0;
	stdin = __create_FILE_struct(0, _STDIO_READ);
	stdout = __create_FILE_struct(1, _STDIO_WRITE);
	stderr = __create_FILE_struct(2, _STDIO_WRITE);

	return 0;
}

int __stdio_deinit(void)
{
    FILE *fNode, *fNext;

    /* Iterate through the list, freeing everything*/
    fNode = __open_file_stack_top;
    _kern_sem_acquire(__open_file_stack_sem_id, 1);
    while(fNode != (FILE*)0)
    {
        fflush(fNode);
        fNext = fNode->next;
        free(fNode->buf);
        _kern_sem_delete(fNode->sid);
        free(fNode);
        fNode = fNext;
    }
    _kern_sem_release(__open_file_stack_sem_id, 1);
    _kern_sem_delete(__open_file_stack_sem_id);

	return 0;
}


FILE *fopen(const char *filename, const char *mode)
{
    FILE* f;
    int sys_flags;
    int flags;
    int fd;

	if(_set_open_flags(mode, &sys_flags, &flags) || (fd = _kern_open(filename, sys_flags)) < 0)
	{
		return (FILE*)0;
	}

    f = __create_FILE_struct(fd, flags);
    if(f == (FILE*)0)
    {
        close(fd);
    }

    return f;
}

FILE *fdopen(int fd, const char *mode)
{
    FILE* f;
    int sys_flags;
    int flags;

	if(_set_open_flags(mode, &sys_flags, &flags))
	{
		return (FILE*)0;
	}

    f = __create_FILE_struct(fd, flags);
    if(f == (FILE*)0)
    {
        close(fd);
    }

    return f;
}

FILE *freopen(const char *filename, const char *mode, FILE *stream)
{
    int sys_flags;
    int flags;
    int fd;

	if(_set_open_flags(mode, &sys_flags, &flags) || (fd = _kern_open(filename, sys_flags)) < 0)
	{
		return (FILE*)0;
	}

	_kern_sem_acquire(stream->sid, 1);
	_flush(stream);
	close(stream->fd);
	stream->fd = fd;
	stream->rpos = stream->buf_pos = 0;
    stream->flags = flags;
	_kern_sem_release(stream->sid, 1);

    return stream;

}

static int _set_open_flags(const char* mode, int* sys_flags, int* flags)
{
	if(!strcmp(mode, "r") || !strcmp(mode, "rb"))
    {
        *sys_flags = O_RDONLY;
        *flags = _STDIO_READ;
    }
    else if(!strcmp(mode, "w") || !strcmp(mode, "wb"))
    {
        *sys_flags = O_WRONLY | O_CREAT | O_TRUNC;
        *flags = _STDIO_WRITE;
    }
    else if(!strcmp(mode, "a") || !strcmp(mode, "ab"))
    {
        *sys_flags = O_WRONLY | O_CREAT | O_APPEND;
        *flags = _STDIO_WRITE;
    }
    else if(!strcmp(mode, "r+") || !strcmp(mode, "rb+") || !strcmp(mode, "r+b"))
    {
        *sys_flags = O_RDWR;
        *flags = _STDIO_READ | _STDIO_WRITE;
    }
    else if(!strcmp(mode, "w+") || !strcmp(mode, "wb+") || !strcmp(mode, "w+b"))
    {
        *sys_flags = O_RDWR | O_CREAT | O_TRUNC;
        *flags = _STDIO_READ | _STDIO_WRITE;
    }
    else if(!strcmp(mode, "a+") || !strcmp(mode, "ab+") || !strcmp(mode, "a+b"))
    {
        *sys_flags = O_RDWR | O_CREAT | O_APPEND;
        *flags = _STDIO_READ | _STDIO_WRITE;
    }
    else
    {
        return -1;
    }
	return 0;
}



long ftell(FILE* stream)
{
	fpos_t p;
	_kern_sem_acquire(stream->sid, 1);
	p = _ftell(stream);
	_kern_sem_release(stream->sid, 1);

	return p;
}

static long _ftell(FILE* stream)
{
	fpos_t p;

	_flush(stream);

	p = _kern_seek(stream->fd, 0, _SEEK_CUR) - ((stream->flags & _STDIO_UNGET) ? 1 : 0);

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

	p = _kern_seek(stream->fd, offset, whence);

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


int fclose(FILE *stream)
{
    int err;
    err = fflush(stream);
    __delete_FILE_struct(stream->fd);
    return err;
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
        _kern_sem_acquire(stream->sid, 1);
		err = _flush(stream);
        _kern_sem_release(stream->sid, 1);
		return err;
    }
}

static int _flush(FILE* stream)
{
	if(stream->buf_pos)
	{
		if(stream->flags & _STDIO_WRITE)
		{
			int err = _kern_write(stream->fd, stream->buf, -1, stream->buf_pos);
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
				dif = _kern_seek(stream->fd, dif, SEEK_CUR);
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


int printf(const char *fmt, ...)
{
	va_list args;
	int i;

    va_start(args, fmt);
    _kern_sem_acquire(stdout->sid, 1);
	i = vfprintf(stdout, fmt, args);
    _kern_sem_release(stdout->sid, 1);
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

int fileno(FILE *stream)
{
	return stream->fd;
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
    if(stream->buf_pos >= stream->buf_size)
    {
        int err = _kern_write(stream->fd, stream->buf, -1, stream->buf_pos);
        if(err < 0)
        {
            errno = EIO;
            stream->flags |= _STDIO_ERROR;
			return EOF;
        }
        stream->buf_pos = 0;
    }
	return (stream->buf[stream->buf_pos++] = (unsigned char)ch);

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
    tmp = str;

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
		if (stream->rpos >= stream->buf_pos)
		{
			int len = _kern_read(stream->fd, stream->buf, -1, stream->buf_size);

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

int sscanf(char const *str, char const *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsscanf(str, fmt, args);
	va_end(args);

	return i;
}


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

int setvbuf(FILE *stream, char *buf, int mode, size_t size)
{
	_kern_sem_acquire(stream->sid, 1);

	_flush(stream);
	if(stream->buf)
		free(stream->buf);
	stream->buf = buf;
	stream->buf_size = size;

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

#endif
