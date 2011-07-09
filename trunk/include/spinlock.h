/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Spinlocks.
 *
 *
**/


#ifndef spinlockH
#define spinlockH

#define SPIN_DEBUG 1

#define spinlock_lock(l,s) hal_spin_lock(l)
#define spinlock_unlock(l,s) hal_spin_unlock(l)
#define spinlock_init(l) hal_spin_init(l)


struct hal_spinlock {
    volatile int lock;
    volatile int ebp;
};

typedef struct hal_spinlock hal_spinlock_t;

void	hal_spin_init(hal_spinlock_t *sl);
void	hal_spin_lock(hal_spinlock_t *sl);
void	hal_spin_unlock(hal_spinlock_t *sl);

static __inline__ int hal_spin_locked(hal_spinlock_t *sl) { return sl->lock; }

#if SPIN_DEBUG
extern int global_lock_entry_count[];
#endif


#endif