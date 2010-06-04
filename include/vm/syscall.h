/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/

#ifndef SYSCALL_H
#define SYSCALL_H

#include "vm/syscall_tools.h"
//#include "vm/systable_id.h"

void phantom_activate_thread(struct pvm_object new_thread);

#endif // SYSCALL_H


