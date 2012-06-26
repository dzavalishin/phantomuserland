/*
 ** Copyright 2001, Travis Geiselbrecht. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "commands.h"
#include "parse.h"
#include "statements.h"
#include "shell_defs.h"
#include "shell_vars.h"
#include "args.h"

#include "main.h"

#include <user/sys_getset.h>

static void getline( char *buf, int size );


int main(int argc, char *argv[], char *envp[])
{
    (void) envp;

    // TODO bring in good malloc/free and implement sbrk()!
    //static char arena[1024*1024];
    //init_malloc( arena, sizeof(arena) );

    printf("Phantom Simple Unix Box Shell is running, pid %d\n", getpid());

#if 0
    printf("ac = %d\n", argc );
    char **avp = argv;
    while( *avp )
    {
        printf("arg = %p\n", *avp );
        printf("arg = '%s'\n", *avp++ );
    }
#endif


    init_vars();
    init_statements();
    init_arguments(argc,argv);

    if(af_script_file_name != NULL)
    {
        run_script(af_script_file_name);
        if(af_exit_after_script) exit(0);
    }

    setvbuf( stdin, 0, _IONBF, 0 );
    //setvbuf( stdout, 0, _IONBF, 0 );

    char buf[1024];

    for(;;) {

        printf("> ");

        getline(buf, sizeof(buf));
        if(strlen(buf) > 0) {
            parse_string(buf);
        }
        buf[0] = '\0';
    }

    return 0;
}


static void getline( char *buf, int size )
{
    int nread = 0;
    char *bp = buf;

    while( nread < size-1 )
    {
        //read( 0, bp, 1 );
        *bp = getchar();

        //printf("!-%c", *bp);
        //printf("%c", *bp);
        //write( 1, bp, 1 );
        putchar(*bp);

        if( *bp == '\n' )
            break;
        bp++;
    }

    *bp = 0;
}


