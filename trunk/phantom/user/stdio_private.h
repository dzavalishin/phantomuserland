#include <stdio.h>


#define BUFSIZ 1024



/* A stack of FILE's currently held by the user*/
FILE* __open_file_stack_top;
/* Semaphore used when adjusting the stack*/
#if STDIO_SEM
sem_id __open_file_stack_sem_id;
#endif // STDIO_SEM

FILE *__create_FILE_struct(int fd, int flags);
int _flush(FILE* stream);



