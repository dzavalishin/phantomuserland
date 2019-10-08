/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Network syscalls - non-kernel version
 * 
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalClasses>
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalMethodWritingGuide>
 *
**/

#include "vm/syscall_net.h"

#include <phantom_libc.h>
#include <vm/syscall_net.h>
#include <vm/alloc.h>

#include "kernel/net.h"


//static int debug_print = 0;


