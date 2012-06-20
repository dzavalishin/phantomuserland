/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Unix syscalls - phantom object land communitcations
 *
 *
**/


#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "funixph"
#include "debug_ext.h"
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10


#include <unix/uufile.h>
#include <unix/uuprocess.h>
#include <kernel/unix.h>
#include <phantom_types.h>

#include "unix/fs_phantom.h"

//int uu_find_fd( uuprocess_t *u, uufile_t * f );



int usys_pmethod( int *err, uuprocess_t *u, const char *m_name, int nfd, int fds[] )
{
}



