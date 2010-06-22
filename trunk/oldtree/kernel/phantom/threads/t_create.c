/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Thread creation.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#define DEBUG_MSG_PREFIX "threads"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include <malloc.h>
#include <string.h>
#include <i386/proc_reg.h>
#include <x86/phantom_page.h>
#include <phantom_libc.h>
#include <kernel/smp.h>
#include "thread_private.h"

static int find_tid(phantom_thread_t *);
static void common_thread_init(phantom_thread_t *t, int stacksize );

static hal_spinlock_t tid_lock; //init?







phantom_thread_t *
phantom_create_thread( void (*func)(void *), void *arg, int flags )
{
    assert( ! (flags & ~CREATION_POSSIBLE_FLAGS) );

    phantom_thread_t *t = calloc(1, sizeof(phantom_thread_t));

    // Can't be run yet
    t->sleep_flags = THREAD_SLEEP_LOCKED; 

    t->tid = find_tid(t);

    common_thread_init(t, 64*1024 );
    //t->priority = THREAD_PRIO_NORM;



    t->start_func_arg = arg;
    t->start_func = func;

    phantom_thread_state_init(t);

    t->thread_flags |= flags;
    // Let it be elegible to run
    t->sleep_flags &= ~THREAD_SLEEP_LOCKED;

    t_enqueue_runq(t);

    return t;

}

int
hal_start_kernel_thread_arg(void (*thread)(void *arg), void *arg)
{
    phantom_thread_t *t = phantom_create_thread( thread, arg, THREAD_FLAG_KERNEL );
    return t->tid;
}


int
hal_start_thread(void (*thread)(void *arg), void *arg, int flags)
{
    phantom_thread_t *t = phantom_create_thread( thread, arg, flags );
    return t->tid;
}








//extern int phantom_start_stack_size;
//extern char phantom_start_stack_start[];

void
phantom_import_main_thread()
{
    phantom_thread_t *t = calloc(1, sizeof(phantom_thread_t));

    // Can't be run yet
    t->sleep_flags = THREAD_SLEEP_LOCKED; 

    int ie = hal_save_cli();
    hal_spin_lock( &tid_lock );
    t->tid = 1;
    assert(phantom_kernel_threads[t->tid] == 0);

    phantom_kernel_threads[t->tid] = t;
    hal_spin_unlock( &tid_lock );
    if(ie) hal_sti();

    // This is not exactly safe! phantom_thread_state_init pushes some stuff on stack and can overrite something
    //t->stack_size = phantom_start_stack_size;
    //t->stack = phantom_start_stack_start;

    //assert(t->stack != 0);

    common_thread_init(t, 64*1024 );

    t->start_func_arg = 0;
    t->start_func = 0;

    phantom_thread_state_init(t);

    t->thread_flags |= THREAD_FLAG_UNDEAD;

    // Let it be elegible to run
    t->sleep_flags &= ~THREAD_SLEEP_LOCKED;

    //GET_CURRENT_THREAD() = t;
    SET_CURRENT_THREAD(t);

    hal_set_thread_name("Main");

}

// Called per each CPU except for boot one.
void
phantom_import_cpu_thread(int ncpu)
{
    // No malloc on new CPU before thread is imported! Malloc has mutex!
    physaddr_t pa; // unused
    phantom_thread_t *t; // = calloc(1, sizeof(phantom_thread_t));
    hal_pv_alloc( &pa, (void **)&t, sizeof(phantom_thread_t) );
    memset( t, sizeof(phantom_thread_t), 0 );

    // Can't be run yet
    t->sleep_flags = THREAD_SLEEP_LOCKED; 

    t->tid = find_tid(t);

    t->start_func_arg = 0;
    t->start_func = 0;

    common_thread_init(t, 64*1024 );
    phantom_thread_state_init(t);

    t->thread_flags |= THREAD_FLAG_UNDEAD;

    // Let it be elegible to run
    t->sleep_flags &= ~THREAD_SLEEP_LOCKED;

    //GET_CURRENT_THREAD() = t;
    SET_CURRENT_THREAD(t);

    char *name = calloc(1, 20);
    snprintf( name, 20, "CPU %d idle", ncpu );

    hal_set_thread_name(name);
    t->priority = THREAD_PRIO_IDLE;

}














static void common_thread_init(phantom_thread_t *t, int stacksize )
{
    //t->thread_flags = 0;
    t->priority = THREAD_PRIO_NORM;

    t->cpu_id = GET_CPU_ID();


    // malloc uses mutex, so we have to use physalloc which is protected with spinlocks
    physaddr_t pa;

    t->stack_size = stacksize;
    //t->stack = calloc( 1, stacksize );
    hal_pv_alloc( &pa, &(t->stack), stacksize+PAGE_SIZE );
    hal_page_control( pa, t->stack, page_unmap, page_noaccess ); // poor man's guard page - TODO support in page fault

    //assert(t->stack != 0);

    t->kstack_size = stacksize;
    //t->kstack = calloc( 1, stacksize );
    hal_pv_alloc( &pa, &(t->kstack), stacksize+PAGE_SIZE );
    hal_page_control( pa, t->kstack, page_unmap, page_noaccess ); // poor man's guard page - TODO support in page fault

    t->kstack_top = t->kstack+t->kstack_size-4; // Why -4?

    //assert(t->kstack != 0);
}



static int last_tid = 2; // Reserve tid 0/1 for idle/main threads


static int find_tid(phantom_thread_t *t)
{
    int maxtries = MAX_THREADS;
    int ie = hal_save_cli();
    hal_spin_lock( &tid_lock );
    do {
        if( phantom_kernel_threads[last_tid] == 0 )
        {
            phantom_kernel_threads[last_tid] = t;
            hal_spin_unlock( &tid_lock );
            if(ie) hal_sti();
            return last_tid;
        }

        last_tid++;

        if(last_tid >= MAX_THREADS)
            last_tid = 2;

        if(maxtries-- < 0)
        {
            hal_spin_unlock( &tid_lock );
            if(ie) hal_sti();
            panic("out of threads");
            return -1; //not reached, panic
        }
    } while(1);
}




