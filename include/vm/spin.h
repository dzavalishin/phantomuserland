/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * VM spinlocks
 *
 *
**/

#ifndef PVM_SPIN_H
#define PVM_SPIN_H


#include <vm/internal_da.h>



void pvm_spin_init( pvm_spinlock_t *ps );
void pvm_spin_lock( pvm_spinlock_t *ps );
void pvm_spin_unlock( pvm_spinlock_t *ps );



#endif // PVM_SPIN_H
