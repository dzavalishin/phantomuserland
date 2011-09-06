/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Wired spinlocks. Can be placed in paged (persistent) memory.
 *
 *
**/


#include <kernel/vm.h>
#include <spinlock.h>


void hal_wired_spin_lock(hal_spinlock_t *l)
{
    wire_page_for_addr( l, sizeof( hal_spinlock_t ) );
    hal_spin_lock(l);
}

void hal_wired_spin_unlock(hal_spinlock_t *l)
{
    hal_spin_unlock(l);
    unwire_page_for_addr( l, sizeof( hal_spinlock_t ) );
}


