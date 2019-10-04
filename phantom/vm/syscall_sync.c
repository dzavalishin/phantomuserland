/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal (native) classes implementation: Synchronization
 * 
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalClasses>
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalMethodWritingGuide>
 *
**/


#define DEBUG_MSG_PREFIX "vm.sysc.win"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

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

void pvm_spin_init( pvm_spinlock_t *ps )
{
    assert( ps != 0 );
    hal_spin_init( &(ps->sl) );
    ps->wait_spin_flag = 0;
}

void pvm_spin_lock( pvm_spinlock_t *ps )
{
    assert( ps != 0 );
    hal_spinlock_t *sl = &(ps->sl);
    volatile int *additional_lock = &(ps->wait_spin_flag);

    volatile int was_locked;
    while(1) {
        // just access it out of spinlock to make sure it is not paged out
        was_locked = *additional_lock; 
        hal_wired_spin_lock(sl);
        was_locked = *additional_lock;
        *additional_lock = 1;
        hal_wired_spin_unlock(sl);

        if(!was_locked) break;

        hal_sleep_msec(1); // yield TODO hal_yield func
    } 
}

void pvm_spin_unlock( pvm_spinlock_t *ps )
{
    assert( ps != 0 );
    hal_spinlock_t *sl = &(ps->sl);
    volatile int *additional_lock = &(ps->wait_spin_flag);
    volatile int was_locked;
    // just access it out of spinlock to make sure it is not paged out
    was_locked = *additional_lock; 
    (void) was_locked;

    hal_wired_spin_lock(sl);
    assert(*additional_lock);
    *additional_lock = 0;
    hal_wired_spin_unlock(sl);
}

// --------- mutex -------------------------------------------------------

// NB - persistent mutexes!
// TODO have a mark - can this mutex be locked at snapshot? No. All of them can.


void vm_mutex_lock( pvm_object_t me, struct data_area_4_thread *tc )
{
#if NEW_VM_SLEEP
    struct data_area_4_mutex *da = pvm_object_da( me, mutex );

    lprintf("warning: unfinished vm_mutex_lock used\r");

    pvm_spin_lock( &(da->lock) );
    pvm_object_t this_thread = pvm_da_to_object(tc);

    if(da->owner_thread == 0)
    {
        da->owner_thread = tc;
        goto done;
    }

    // TODO if taken by us?
    if(da->owner_thread == 0)
    {
        lprintf("vm_mutex_lock retake\r");
        goto done;
    }

    // Mutex is already taken not by us, fall asleep now

    assert(!pvm_isnull(this_thread));
    assert(pvm_object_class_exactly_is( this_thread, pvm_get_thread_class() ) );

    ref_inc_o(this_thread); // ?? hack? TODO refdec!
    pvm_set_ofield( da->waiting_threads_array, da->nwaiting++, this_thread );

//#warning have SYSCALL_PUT_THIS_THREAD_ASLEEP unlock the spinlock!
    //VM_SPIN_UNLOCK(da->poor_mans_pagefault_compatible_spinlock);
    SYSCALL_PUT_THIS_THREAD_ASLEEP(&da->lock);
    return;

done:
    pvm_spin_unlock( &(da->lock) );
#else
    SYSCALL_THROW_STRING("Not this way");
#endif
}

errno_t vm_mutex_unlock( pvm_object_t me, struct data_area_4_thread *tc )
{
#if NEW_VM_SLEEP
    struct data_area_4_mutex *da = pvm_object_da( me, mutex );

    int ret = 0;

    pvm_spin_lock( &(da->lock) );

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
    assert(pvm_object_class_exactly_is( next_thread, pvm_get_thread_class() ) );

    da->owner_thread = pvm_object_da( next_thread, thread );
    SYSCALL_WAKE_THREAD_UP( da->owner_thread );

done:
    pvm_spin_unlock( &(da->lock) );
    return ret;
#else
    SYSCALL_THROW_STRING("Not this way");
#endif
}


static int si_mutex_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "mutex" ));
}


static int si_mutex_8_lock( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    vm_mutex_lock( me, tc );
    SYSCALL_RETURN_NOTHING;
}

static int si_mutex_9_unlock( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    //struct data_area_4_mutex *da = pvm_object_da( me, mutex );
    //(void)da;

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

static int si_mutex_10_trylock( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    //struct data_area_4_mutex *da = pvm_object_da( me, mutex );

    // No locking in syscalls!!
    //SYSCALL_RETURN(pvm_create_int_object( pthread_mutex_trylock(&(da->mutex)) ));
    SYSCALL_THROW_STRING( "mutex si_mutex_10_trylock - not impl" );
    //SYSCALL_RETURN_NOTHING;
}


syscall_func_t  syscall_table_4_mutex[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_mutex_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_mutex_8_lock,               &si_mutex_9_unlock,
    &si_mutex_10_trylock,           &invalid_syscall,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
    // 16

};
DECLARE_SIZE(mutex);



// --------- cond -------------------------------------------------------

static int si_cond_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( ".internal.cond" ));
}


static int si_cond_8_wait( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;

    CHECK_PARAM_COUNT(1);

    pvm_object_t mutex = args[0];

#if NEW_VM_SLEEP
    struct data_area_4_cond *da = pvm_object_da( me, cond );
    //SYSCALL_THROW_STRING("Not this way");
    lprintf("unimplemented vm_cond_wait used\r");

    pvm_spin_lock( &(da->lock) );
    pvm_object_t this_thread = pvm_da_to_object(tc);

    assert(!pvm_isnull(this_thread));
    assert(pvm_object_class_exactly_is( this_thread, pvm_get_thread_class() ) );

    ref_inc_o(this_thread); // ?? hack? TODO ref dec?!
    pvm_set_ofield( da->waiting_threads_array, da->nwaiting++, this_thread );

    tc->cond_mutex = mutex;
    SYSCALL_PUT_THIS_THREAD_ASLEEP(&da->lock);

    vm_mutex_unlock( mutex, tc );
    SYS_FREE_O(mutex);

    //pvm_spin_unlock( &(da->lock) );
    SYSCALL_RETURN_NOTHING;
#else
    SYS_FREE_O(mutex);
    SYSCALL_THROW_STRING("Not this way");
#endif
}

static int si_cond_9_twait( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    struct data_area_4_cond *da = pvm_object_da( me, cond );
    (void)da;

    SYSCALL_THROW_STRING( "timed wait not impl" );
}

static int si_cond_10_broadcast( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;

    CHECK_PARAM_COUNT(0);

    //pvm_object_t mutex = args[0];

#if NEW_VM_SLEEP
    struct data_area_4_cond *da = pvm_object_da( me, cond );
    //SYSCALL_THROW_STRING("Not this way");
    lprintf("unimplemented vm_cond_wait used\r");
    pvm_spin_lock( &(da->lock) );

    while(da->nwaiting > 0)
    {
        // TODO takes last, must take first
        pvm_object_t next_thread = pvm_get_ofield( da->waiting_threads_array, --da->nwaiting );

        assert(!pvm_isnull(next_thread));
        assert(pvm_object_class_exactly_is( next_thread, pvm_get_thread_class() ) );

        struct data_area_4_thread *thda = pvm_object_da( next_thread, thread );
        SYSCALL_WAKE_THREAD_UP( thda );
    }

    pvm_spin_unlock( &(da->lock) );
    SYSCALL_RETURN_NOTHING;
#else
    //SYS_FREE_O(mutex);
    SYSCALL_THROW_STRING("Not this way");
#endif

}

static int si_cond_11_signal( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;

    CHECK_PARAM_COUNT(0);

    //pvm_object_t mutex = args[0];

#if NEW_VM_SLEEP
    struct data_area_4_cond *da = pvm_object_da( me, cond );
    //SYSCALL_THROW_STRING("Not this way");
    lprintf("unimplemented vm_cond_wait used\r");
    pvm_spin_lock( &(da->lock) );

    if(da->nwaiting > 0)
    {
        // TODO takes last, must take first
        pvm_object_t next_thread = pvm_get_ofield( da->waiting_threads_array, --da->nwaiting );

        assert(!pvm_isnull(next_thread));
        assert(pvm_object_class_exactly_is( next_thread, pvm_get_thread_class() ) );

        struct data_area_4_thread *thda = pvm_object_da( next_thread, thread );
        SYSCALL_WAKE_THREAD_UP( thda );
    }

    pvm_spin_unlock( &(da->lock) );
    SYSCALL_RETURN_NOTHING;
#else
    //SYS_FREE_O(mutex);
    SYSCALL_THROW_STRING("Not this way");
#endif
}


syscall_func_t  syscall_table_4_cond[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_cond_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_cond_8_wait,                &si_cond_9_twait,
    &si_cond_10_broadcast,          &si_cond_11_signal,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
    // 16

};
DECLARE_SIZE(cond);


// --------- sema -------------------------------------------------------

static int si_sema_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( ".internal.sema" ));
}


static int si_sema_8_acquire( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
/*
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
*/
    SYSCALL_THROW_STRING("Not this way");
//#endif
}

static int si_sema_9_tacquire( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    struct data_area_4_sema *da = pvm_object_da( me, sema );
    (void)da;

    SYSCALL_THROW_STRING( "timed acquire not impl" );


    SYSCALL_RETURN_NOTHING;
}

// Idea is to clear sema before servising request and wait on it (acquire) after
static int si_sema_10_zero( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_sema *da = pvm_object_da( me, sema );

    // TODO in spinlock
    if( da->sem_value > 0 )
        da->sem_value = 0;

    SYSCALL_RETURN_NOTHING;
}

static int si_sema_11_release( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
/*
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
*/
    SYSCALL_THROW_STRING("Not this way");
//#endif
}


syscall_func_t  syscall_table_4_sema[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_void_3_clone,
    &si_void_4_equals,              &si_sema_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_sema_8_acquire,             &si_sema_9_tacquire,
    &si_sema_10_zero,               &si_sema_11_release,
    &invalid_syscall,               &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode
    // 16

};
DECLARE_SIZE(sema);

