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
#include <threads.h>
#include <hal.h>

void hal_wired_spin_lock(hal_spinlock_t *l)
{
    l->ei = hal_save_cli();
    wire_page_for_addr( l, sizeof( hal_spinlock_t ) );
    hal_disable_preemption();
    hal_spin_lock(l);
}

void hal_wired_spin_unlock(hal_spinlock_t *l)
{
    hal_spin_unlock(l);
    hal_enable_preemption();
    unwire_page_for_addr( l, sizeof( hal_spinlock_t ) );
    if( l && (l->ei) ) hal_sti();
}



void hal_spin_lock_cli(hal_spinlock_t *sl)
{
    if(sl) // Scheduler sometimes calls us will sl == 0
    {
        sl->ei = hal_save_cli();
        hal_spin_lock(sl);
    }
}

void hal_spin_unlock_sti(hal_spinlock_t *sl)
{
    if(sl) // Scheduler sometimes calls us will sl == 0
    {
        hal_spin_unlock(sl);
        if( sl->ei ) hal_sti();
    }
}

