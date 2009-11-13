
//---------------------------------------------------------------------------
#ifndef spinlockH
#define spinlockH

//#include "except.h"
//#include "hal.h"

//#error dont use me, use hal

#define spinlock_lock(l,s) hal_spin_lock(l)
#define spinlock_unlock(l,s) hal_spin_unlock(l)
#define spinlock_init(l) hal_spin_init(l)

// ------------------------------------------------------------------------------------------
// SPINLOCKS

struct hal_spinlock {
	volatile int lock;
};

typedef struct hal_spinlock hal_spinlock_t;

void 					hal_spin_init(hal_spinlock_t *sl);
void 					hal_spin_lock(hal_spinlock_t *sl);
void 					hal_spin_unlock(hal_spinlock_t *sl);



/*
#define _QUOTE(x) # x
#define QUOTE(x) _QUOTE(x)
#define __FILE__LINE__ __FILE__ "(" QUOTE(__LINE__) ")"
*/



#endif
