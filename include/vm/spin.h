/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * VM spinlock, mutex, cond
 *
 *
**/

#ifndef PVM_SPIN_H
#define PVM_SPIN_H

#include <vm/internal_da.h>

void pvm_spin_init( pvm_spinlock_t *ps );
void pvm_spin_lock( pvm_spinlock_t *ps );
void pvm_spin_unlock( pvm_spinlock_t *ps );


void vm_mutex_lock( pvm_object_t me, struct data_area_4_thread *tc );
errno_t vm_mutex_unlock( pvm_object_t me, struct data_area_4_thread *tc );

void pvm_cond_wait( pvm_object_t me, struct data_area_4_thread *tc, pvm_object_t mutex );
void pvm_cond_signal( pvm_object_t me );
void pvm_cond_broadcast( pvm_object_t me );

#endif // PVM_SPIN_H
