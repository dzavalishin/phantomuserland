/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Outdated?
 *
**/

#ifndef SYSCALL_H
#define SYSCALL_H

#include <vm/syscall_tools.h>

void phantom_activate_thread(pvm_object_t new_thread);

// returns 1 for regular return, 0 for throw. in both cases must push ret val / throwable
int vm_syscall_block( pvm_object_t this, struct data_area_4_thread *tc, pvm_object_t (*syscall_worker)( pvm_object_t this, struct data_area_4_thread *tc, int nmethod, pvm_object_t arg ) );


#endif // SYSCALL_H


