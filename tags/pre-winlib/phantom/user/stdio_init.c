#include <user/sys_fio.h>
#include <string.h>
#include <stdio.h>

#include "stdio_private.h"

int __stdio_init(void)
{
#if STDIO_SEM
    /* Create semaphore*/
    __open_file_stack_sem_id = _kern_sem_create(1, "__open_file_stack");
#endif // STDIO_SEM

    /*initialize stack*/
    __open_file_stack_top = (FILE*)0;

    stdin = __create_FILE_struct(0, _STDIO_READ);
    stdout = __create_FILE_struct(1, _STDIO_WRITE);
    stderr = __create_FILE_struct(2, _STDIO_WRITE);

    stdin->buf_mode = _IOLBF;
    stdout->buf_mode = _IOLBF;
    stderr->buf_mode = _IOLBF;

    return 0;
}

int __stdio_deinit(void)
{
    FILE *fNode, *fNext;

    /* Iterate through the list, freeing everything*/
    fNode = __open_file_stack_top;
#if STDIO_SEM
    _kern_sem_acquire(__open_file_stack_sem_id, 1);
#endif // STDIO_SEM
    while(fNode != (FILE*)0)
    {
        fflush(fNode);
        fNext = fNode->next;
        free(fNode->buf);
#if STDIO_SEM
        _kern_sem_delete(fNode->sid);
#endif // STDIO_SEM
        free(fNode);
        fNode = fNext;
    }
#if STDIO_SEM
    _kern_sem_release(__open_file_stack_sem_id, 1);
    _kern_sem_delete(__open_file_stack_sem_id);
#endif // STDIO_SEM

    return 0;
}

