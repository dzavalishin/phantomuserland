#include <user/sys_fio.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "stdio_private.h"


static int _set_open_flags(const char* mode, int* sys_flags, int* flags);


FILE *__create_FILE_struct(int fd, int flags)
{
    FILE *f;

    /* Allocate the FILE*/
    f = (FILE *)calloc(sizeof(FILE),1);
    if(!f)
        return (FILE *)0;

    /* Allocate the buffer*/
    f->buf = (unsigned char *)malloc( BUFSIZ * sizeof(char));
    if(!f->buf)
    {
        free(f);
        return (FILE *)0;
    }

#if STDIO_SEM
    char name[32];
    /* Create a semaphore*/
    snprintf(name, sizeof(name), "%d :FILE", fd);
    f->sid = _kern_sem_create(1, name);
    if(f->sid < 0)
    {
        free(f->buf);
        free(f);
        return (FILE *)0;
    }
#endif // STDIO_SEM

    /* Fill in FILE values*/
    f->rpos = 0;
    f->buf_pos = 0;
    f->buf_size = BUFSIZ ;
    f->fd = fd;
    f->flags = flags;
    f->buf_mode = _IOFBF;

    /* Setup list*/
    f->next = __open_file_stack_top;

    /* Put the FILE in the list*/
#if STDIO_SEM
    _kern_sem_acquire(__open_file_stack_sem_id, 1);
    __open_file_stack_top = f;
    _kern_sem_release(__open_file_stack_sem_id, 1);
#else
    __open_file_stack_top = f;
#endif // STDIO_SEM
    return f;
}

static int __delete_FILE_struct(int fd)
{
    FILE *fNode, *fPrev;
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

#if STDIO_SEM
    /* Wait for the lock */
    sem_id sid;
    sid = fNode->sid;
    _kern_sem_acquire(sid, 1);
#endif

    /* free the FILE space/semaphore*/
    close(fNode->fd);
    free(fNode->buf);
    free(fNode);

#if STDIO_SEM
    /* Do we need to release before we delete? */
    _kern_sem_release(sid, 1);
    _kern_sem_delete(sid);
#endif

#if STDIO_SEM
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
#else
    /* Remove it from the list*/
    if(fNode == __open_file_stack_top)
        __open_file_stack_top = __open_file_stack_top->next;
    else
        fPrev->next = fNode->next;
#endif

    /* Free the space*/
    free(fNode);

    return 0;
}





















FILE *fopen(const char *filename, const char *mode)
{
    FILE* f;
    int sys_flags;
    int flags;
    int fd;

    if(_set_open_flags(mode, &sys_flags, &flags) || ((fd = open(filename, sys_flags)) < 0) )
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

    if(_set_open_flags(mode, &sys_flags, &flags) || (fd = open(filename, sys_flags)) < 0)
    {
        return (FILE*)0;
    }

#if STDIO_SEM
    _kern_sem_acquire(stream->sid, 1);
#endif // STDIO_SEM
    _flush(stream);
    close(stream->fd);
    stream->fd = fd;
    stream->rpos = stream->buf_pos = 0;
    stream->flags = flags;
#if STDIO_SEM
    _kern_sem_release(stream->sid, 1);
#endif // STDIO_SEM

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










int fclose(FILE *stream)
{
    int err;
    err = fflush(stream);
    __delete_FILE_struct(stream->fd);
    return err;
}


