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
    volatile int ei; // Interrupts status before lock - NB - used by wired_spin only
};

typedef struct hal_spinlock hal_spinlock_t;

void	hal_spin_init(hal_spinlock_t *sl);
void	hal_spin_lock(hal_spinlock_t *sl);
void	hal_spin_unlock(hal_spinlock_t *sl);

//void	hal_spin_lock(hal_spinlock_t *sl) __attribute__((deprecated));
//void	hal_spin_unlock(hal_spinlock_t *sl) __attribute__((deprecated));

// Turns off interrupts too
void    hal_wired_spin_lock(hal_spinlock_t *l);
void    hal_wired_spin_unlock(hal_spinlock_t *l);

void    hal_spin_lock_cli(hal_spinlock_t *sl);
void    hal_spin_unlock_sti(hal_spinlock_t *sl);

static __inline__ int hal_spin_locked(const hal_spinlock_t *sl) { return sl->lock; }
static __inline__ int hal_spin_is_locked(const hal_spinlock_t *sl) { return sl->lock; }


#if SPIN_DEBUG && !HAVE_SMP
extern int global_lock_entry_count[];
#endif

void check_global_lock_entry_count();

#define ASSERT_LOCKED_SPIN(sl) assert(hal_spin_is_locked(sl))



struct pvm_spinlock {
	hal_spinlock_t	sl;
	int wait_spin_flag;
};

typedef struct pvm_spinlock pvm_spinlock_t;


#endif













