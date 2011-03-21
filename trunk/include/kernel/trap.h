/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Trap/interrupt context structure. See arch/...
 *
 *
**/


#ifndef GENERAL_TRAP_H
#define GENERAL_TRAP_H


#ifdef ARCH_arm
#include <arm/trap.h>
#else

#ifdef ARCH_ia32
#include <i386/trap.h>

#else

#error No arch?

#endif
#endif




int (*phantom_trap_handlers[ARCH_N_TRAPS])(struct trap_state *ts);



void 		dump_ss(struct trap_state *st);
const char *	trap_name(unsigned int trapnum);
int 		trap_panic(struct trap_state *ts);

// Check if current thread is usermode and convert trap to thread kill
void 		phantom_check_user_trap( struct trap_state *ts );

//! Return signal number for this kind of trap - machdep
int             trap2signo( struct trap_state *ts );


#endif // GENERAL_TRAP_H


