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

#define SIG_PUSH(__v_) *((u_int32_t *)(mina+(--(st->esp)))) = (__v_)


errno_t sig_deliver( uuprocess_t *u, struct trap_state *st, int nsig, void *handler )
{
    u_int32_t   old_eip = st->eip;
    //u_int32_t   old_esp = st->esp;

    addr_t mina = (addr_t)u->mem_start;

    // Push args backwards
    SIG_PUSH(0); // one more zero arg for any case :)
    SIG_PUSH(nsig);

    // Push cur IP
    SIG_PUSH(old_eip);
    // Change IP to handler address
    st->eip = (addr_t)handler;

    return 0;
}

