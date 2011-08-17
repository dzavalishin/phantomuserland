/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Signals - machdep signal delivery code. Called before 
 * returning to user from syscall or interrupt.
 *
**/

#define DEBUG_MSG_PREFIX "sigdeliver"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_types.h>
#include <phantom_libc.h>

#include <kernel/trap.h>
#include <unix/uusignal.h>
#include <unix/uuprocess.h>

//#define SIG_PUSH(__v_) *((u_int32_t *)(mina+(--(st->esp)))) = (__v_)


errno_t sig_deliver( uuprocess_t *u, struct trap_state *st, int nsig, void *handler )
{
	SHOW_ERROR0( 0, "Signal delivery is not implemented!");
    return 0;
}

