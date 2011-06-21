/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Pipes test
 *
 *
**/

//#include <user/sys_fio.h>
#include <sys/unistd.h>
#include <phantom_libc.h>

#include "test.h"

#define BSIZE 256

static char data[] = "abcdefg";

int do_test_pipe(const char *test_parm)
{
    (void) test_parm;

    char buf[BSIZE];
    int rc;

    int fd[2];

    test_check_eq(pipe(fd),0);

    printf("pipe write\n");
    //test_check_eq(write(fd[0], data, sizeof(data)), sizeof(data));
    rc = write(fd[1], data, sizeof(data));
    if( rc != sizeof(data) )
    {
        printf("pipe write expected %d, got %d\n", sizeof(data), rc );
        test_fail_msg( -1, "pipe write" );
    }

    printf("pipe read\n");
    test_check_eq(read(fd[0], buf, sizeof(buf)), sizeof(data));


    test_check_eq(0, strcmp( data, buf ));

    printf("pipe close\n");
    rc = close(fd[0]);
    if(rc)
    {
        printf("pipe close expected 0, got %d\n", rc );
        test_fail_msg( -1, "pipe close" );
    }

    test_check_eq(close(fd[1]),0);

    printf("pipe done\n");

    return 0;
}



