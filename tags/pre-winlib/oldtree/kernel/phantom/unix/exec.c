/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Unix executables pool - TODO
 *
 *
**/


#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "exec"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include <kernel/unix.h>
#include <kernel/pool.h>
#include <kernel/libkern.h>

#include <unix/uuprocess.h>

static void * 	do_exe_create(void *arg);
static void  	do_exe_destroy(void *arg);

void init_unix_exec()
{
    ep = create_pool();
    ep->init = do_exe_create;
    ep->destroy = do_exe_destroy;
}




#endif // HAVE_UNIX

