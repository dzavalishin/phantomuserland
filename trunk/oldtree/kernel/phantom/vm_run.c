/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * VM - run VM callback from kernel.
 *
 *
**/

#define DEBUG_MSG_PREFIX "vm_run"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>


// TODO thread pool? Reuse finishing threads?

/**
 *
 * Run callback.
 *
**/



