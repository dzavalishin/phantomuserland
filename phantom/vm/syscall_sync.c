/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Synchronization syscalls
 *
 *
**/


#include <phantom_libc.h>
#include <time.h>
#include <threads.h>

#include <kernel/snap_sync.h>
#include <kernel/vm.h>
#include <kernel/atomic.h>
#include <kernel/debug.h>

#include <vm/syscall.h>
#include <vm/object.h>
#include <vm/root.h>
#include <vm/exec.h>
#include <vm/bulk.h>
#include <vm/alloc.h>
#include <vm/internal.h>

#include <vm/p2c.h>

#include <vm/wrappers.h>


static int debug_print = 0;




// --------- mutex -------------------------------------------------------

#if OLD_VM_SLEEP
#  warning paging can switch us off with "spin" locked, use kern mutex
#endif
// NB - persistent mutexes!
// TODO have a mark - can this mutex be locked at snapshot


void vm_mutex_lock( pvm_object_t me, struct data_area_4_thread *tc )
{
#if OLD_VM_SLEEP
    struct data_area_4_mutex *da = pvm_object_da( me, mutex );

    VM_SPIN_LOCK(da->poor_mans_pagefault_compatible_spinlock);

    if(da->owner_thread == 0)
    {
        da->owner_thread = tc;
        goto done;
    }

    // Mutex is taken, fall asleep now
    pvm_object_t this_thread = pvm_da_to_object(tc);

    assert(!pvm_isnull(this_thread));
    assert(pvm_object_class_is( this_thread, pvm_get_thread_class() ) );

    ref_inc_o(this_thread); // ?? hack?
    pvm_set_ofield( da->waiting_threads_array, da->nwaiting++, this_thread );

//#warning have SYSCALL_PUT_THIS_THREAD_ASLEEP unlock the spinlock!
    //VM_SPIN_UNLOCK(da->poor_mans_pagefault_compatible_spinlock);
    SYSCALL_PUT_THIS_THREAD_ASLEEP(&da->poor_mans_pagefault_compatible_spinlock);
    return;

done:
    VM_SPIN_UNLOCK(da->poor_mans_pagefault_compatible_spinlock);
#else
    //SYSCALL_THROW_STRING("Not this way");
    lprintf("unimplemented vm_mutex_lock used\r");
#endif
}

errno_t vm_mutex_unlock( pvm_object_t me, struct data_area_4_thread *tc )
{
#if OLD_VM_SLEEP
    struct data_area_4_mutex *da = pvm_object_da( me, mutex );

    int ret = 0;

    VM_SPIN_LOCK(da->poor_mans_pagefault_compatible_spinlock);

    if(da->owner_thread != tc)
    {
        ret = EINVAL;
        goto done;
    }

    // I'm only one here
    if( da->nwaiting == 0 )
    {
        da->owner_thread = 0;
        goto done;
    }

    // TODO takes last, must take first
    pvm_object_t next_thread = pvm_get_ofield( da->waiting_threads_array, --da->nwaiting );

    assert(!pvm_isnull(next_thread));
    assert(pvm_object_class_is( next_thread, pvm_get_thread_class() ) );


    da->owner_thread = pvm_object_da( next_thread, thread );
    SYSCALL_WAKE_THREAD_UP( da->owner_thread );

done:
    VM_SPIN_UNLOCK(da->poor_mans_pagefault_compatible_spinlock);
    return ret;
#else
    SYSCALL_THROW_STRING("Not this way");
#endif
}


static int si_mutex_5_tostring(struct pvm_object o, struct data_area_4_thread *tc )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "mutex" ));
}


static int si_mutex_8_lock(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    vm_mutex_lock( me, tc );
    SYSCALL_RETURN_NOTHING;
}

static int si_mutex_9_unlock(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    //struct data_area_4_mutex *da = pvm_object_da( me, mutex );
    //(void)da;

    // No locking in syscalls!!
    //pthread_mutex_unlock(&(da->mutex));

    errno_t rc = vm_mutex_unlock( me, tc );
    switch(rc)
    {
    case EINVAL:
        printf("mutex unlock - not owner");
        SYSCALL_THROW_STRING( "mutex unlock - not owner" );
        // unreached
        break;

    default: break;
    }

    SYSCALL_RETURN_NOTHING;
}

static int si_mutex_10_trylock(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    //struct data_area_4_mutex *da = pvm_object_da( me, mutex );

    // No locking in syscalls!!
    //SYSCALL_RETURN(pvm_create_int_object( pthread_mutex_trylock(&(da->mutex)) ));

    SYSCALL_RETURN_NOTHING;
}


syscall_func_t	syscall_table_4_mutex[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_mutex_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_mutex_8_lock,     	    &si_mutex_9_unlock,
    &si_mutex_10_trylock, 	    &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
    // 16

};
DECLARE_SIZE(mutex);



// --------- cond -------------------------------------------------------

static int si_cond_5_tostring(struct pvm_object o, struct data_area_4_thread *tc )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( ".internal.cond" ));
}


static int si_cond_8_wait(struct pvm_object me, struct data_area_4_thread *tc )
{
    (void)me;
    DEBUG_INFO;
    struct data_area_4_cond *da = pvm_object_da( me, cond );
    (void)da;

    // No locking in syscalls!!
    //pthread_cond_wait(&(da->cond));

    //SYSCALL_PUT_THIS_THREAD_ASLEEP();
    SYSCALL_THROW_STRING( "wait not impl" );

    SYSCALL_RETURN_NOTHING;
}

static int si_cond_9_twait(struct pvm_object me, struct data_area_4_thread *tc )
{
    (void)me;
    DEBUG_INFO;
    struct data_area_4_cond *da = pvm_object_da( me, cond );
    (void)da;

    SYSCALL_THROW_STRING( "timed wait not impl" );

    // No locking in syscalls!!
    //pthread_cond_timedwait(&(da->cond));

    //SYSCALL_PUT_THIS_THREAD_ASLEEP();


    SYSCALL_RETURN_NOTHING;
}

static int si_cond_10_broadcast(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_cond *da = pvm_object_da( me, cond );
    (void)da;

    // No locking in syscalls!!
    //pthread_cond_broadcast(&(da->cond));

    //SYSCALL_WAKE_THREAD_UP(thread)

    SYSCALL_RETURN_NOTHING;
}

static int si_cond_11_signal(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_cond *da = pvm_object_da( me, cond );
    (void)da;

    // No locking in syscalls!!
    //pthread_cond_signal(&(da->cond));

    //SYSCALL_WAKE_THREAD_UP(thread)

    SYSCALL_RETURN_NOTHING;
}


syscall_func_t	syscall_table_4_cond[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_cond_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_cond_8_wait,      	    &si_cond_9_twait,
    &si_cond_10_broadcast,	    &si_cond_11_signal,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
    // 16

};
DECLARE_SIZE(cond);


// --------- sema -------------------------------------------------------

static int si_sema_5_tostring(struct pvm_object o, struct data_area_4_thread *tc )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( ".internal.sema" ));
}


static int si_sema_8_acquire(struct pvm_object me, struct data_area_4_thread *tc )
{
#if OLD_VM_SLEEP
    DEBUG_INFO;
    struct data_area_4_sema *da = pvm_object_da( me, sema );
    VM_SPIN_LOCK(da->poor_mans_pagefault_compatible_spinlock);

    while( da->sem_value <= 0 )
    {
        // Sema is busy, fall asleep now
        pvm_object_t this_thread = pvm_da_to_object(tc);

        assert(!pvm_isnull(this_thread));
        assert(pvm_object_class_is( this_thread, pvm_get_thread_class() ) );

        pvm_set_ofield( da->waiting_threads_array, da->nwaiting++, this_thread );

//#warning have SYSCALL_PUT_THIS_THREAD_ASLEEP unlock the spinlock!
        //VM_SPIN_UNLOCK(da->poor_mans_pagefault_compatible_spinlock);
        SYSCALL_PUT_THIS_THREAD_ASLEEP(&da->poor_mans_pagefault_compatible_spinlock);
        VM_SPIN_LOCK(da->poor_mans_pagefault_compatible_spinlock);
    }

    da->sem_value--;
    da->owner_thread = tc;

    VM_SPIN_UNLOCK(da->poor_mans_pagefault_compatible_spinlock);
    SYSCALL_RETURN_NOTHING;
#else
    SYSCALL_THROW_STRING("Not this way");
#endif
}

static int si_sema_9_tacquire(struct pvm_object me, struct data_area_4_thread *tc )
{
    (void)me;
    DEBUG_INFO;
    struct data_area_4_sema *da = pvm_object_da( me, sema );
    (void)da;

    SYSCALL_THROW_STRING( "timed acquire not impl" );


    SYSCALL_RETURN_NOTHING;
}

// Idea is to clear sema before servising request and wait on it (acquire) after
static int si_sema_10_zero(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_sema *da = pvm_object_da( me, sema );

    if( da->sem_value > 0 )
        da->sem_value = 0;

    SYSCALL_RETURN_NOTHING;
}

static int si_sema_11_release(struct pvm_object me, struct data_area_4_thread *tc )
{
#if OLD_VM_SLEEP
    DEBUG_INFO;
    struct data_area_4_sema *da = pvm_object_da( me, sema );
    VM_SPIN_LOCK(da->poor_mans_pagefault_compatible_spinlock);

    da->sem_value++;

    if( da->nwaiting > 0 )
    {
        // Wakeup one
        // TODO takes last, must take first
        pvm_object_t next_thread = pvm_get_ofield( da->waiting_threads_array, --da->nwaiting );

        assert(!pvm_isnull(next_thread));
        assert(pvm_object_class_is( next_thread, pvm_get_thread_class() ) );

        da->owner_thread = pvm_object_da( next_thread, thread );
        SYSCALL_WAKE_THREAD_UP( da->owner_thread );
    }

    VM_SPIN_UNLOCK(da->poor_mans_pagefault_compatible_spinlock);
    SYSCALL_RETURN_NOTHING;
#else
    SYSCALL_THROW_STRING("Not this way");
#endif
}


syscall_func_t	syscall_table_4_sema[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_sema_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_sema_8_acquire,      	    &si_sema_9_tacquire,
    &si_sema_10_zero,	            &si_sema_11_release,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
    // 16

};
DECLARE_SIZE(sema);

