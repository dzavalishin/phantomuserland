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
#include "../debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include <malloc.h>
#include <string.h>
#include <i386/proc_reg.h>
#include <phantom_libc.h>
#include "thread_private.h"

static int find_tid(phantom_thread_t *);

#if USE_FORK_LUKE
static int t_prepare_fork();
#endif


phantom_thread_t *
phantom_create_thread( void (*func)(void *), void *arg, int flags )
{
    assert( ! (flags & ~CREATION_POSSIBLE_FLAGS) );


#if USE_FORK_LUKE
    phantom_thread_t *t = calloc(1, sizeof(phantom_thread_t));

    // Can't be run yet
    t->sleep_flags = THREAD_SLEEP_LOCKED; 
    t->thread_flags = 0;

    t->tid = find_tid(t);
    t->name = "?";

    //t->priority = THREAD_PRIO_NORM;

    int ssize = 64*1024;

    t->stack_size = ssize;
    t->stack = calloc( 1, ssize );

    assert(t->stack != 0);

    t->start_func_arg = arg;
    t->start_func = func;

    phantom_thread_state_init(t);

    // Let it be elegible to run
    //t->sleep_flags &= ~THREAD_SLEEP_LOCKED;
    //t_enqueue_runq(t);

    GET_CURRENT_THREAD()->child_tid = t->tid;


    GET_CURRENT_THREAD()->thread_flags |= THREAD_FLAG_PARENT;
    t->thread_flags |= THREAD_FLAG_CHILD;


    if(t_prepare_fork())
    {
        // child - NEW STACK, no local vars are accessible
        phantom_thread_t *me = GET_CURRENT_THREAD();

        void (*sstart)(void *) = me->start_func;

        sstart(me->start_func_arg);
        panic("thread returned");
    }

    // parent

    return t;
#else
    //phantom_thread_t *t = find_thread();
    phantom_thread_t *t = calloc(1, sizeof(phantom_thread_t));

    // Can't be run yet
    t->sleep_flags = THREAD_SLEEP_LOCKED; 
    t->thread_flags = 0;

    t->tid = find_tid(t);

    //t->priority = THREAD_PRIO_NORM;

    int ssize = 64*1024;
    // TODO guard page
    t->stack_size = ssize;
    t->stack = calloc( 1, ssize );

    assert(t->stack != 0);

    // TODO guard page
    t->kstack_size = ssize;
    t->kstack = calloc( 1, ssize );
    t->kstack_top = t->kstack+t->kstack_size-4; // Why -4?

    assert(t->kstack != 0);

    t->start_func_arg = arg;
    t->start_func = func;

    phantom_thread_state_init(t);

    t->thread_flags |= flags;
    // Let it be elegible to run
    t->sleep_flags &= ~THREAD_SLEEP_LOCKED;

    t_enqueue_runq(t);

    return t;
#endif
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



static int last_tid = 2; // Reserve tid 0/1 for idle/main threads

static hal_spinlock_t tid_lock; //init?

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





extern int phantom_start_stack_size;
extern char phantom_start_stack_start[];

void
phantom_import_main_thread()
{
    phantom_thread_t *t = calloc(1, sizeof(phantom_thread_t));

    // Can't be run yet
    t->sleep_flags = THREAD_SLEEP_LOCKED; 
    t->thread_flags = 0;

    int ie = hal_save_cli();
    hal_spin_lock( &tid_lock );
    t->tid = 1;
    assert(phantom_kernel_threads[t->tid] == 0);

    phantom_kernel_threads[t->tid] = t;
    hal_spin_unlock( &tid_lock );
    if(ie) hal_sti();

    t->stack_size = phantom_start_stack_size;
    t->stack = phantom_start_stack_start;

    assert(t->stack != 0);

    t->start_func_arg = 0;
    t->start_func = 0;

    phantom_thread_state_init(t);

    t->priority = THREAD_PRIO_NORM;

    // Let it be elegible to run
    t->sleep_flags &= ~THREAD_SLEEP_LOCKED;

    //GET_CURRENT_THREAD() = t;
    SET_CURRENT_THREAD(t);

    hal_set_thread_name("Main");

}












#if USE_FORK_LUKE

static hal_spinlock_t 	forkLock;
static int              retcount;


static int t_prepare_fork()
{
    hal_spin_lock(&forkLock);
    retcount = 2;



    GET_CURRENT_THREAD()->thread_flags |= THREAD_FLAG_PREFORK;

    while(GET_CURRENT_THREAD()->thread_flags & THREAD_FLAG_PREFORK)
        ; // TODO need softint here

    if(GET_CURRENT_THREAD()->thread_flags & THREAD_FLAG_CHILD)
    {
        GET_CURRENT_THREAD()->thread_flags &= ~THREAD_FLAG_CHILD;
        retcount--;
        if(retcount == 0)
            hal_spin_unlock(&forkLock);
        return 1;
    }

    if(GET_CURRENT_THREAD()->thread_flags & THREAD_FLAG_PARENT)
    {
        GET_CURRENT_THREAD()->thread_flags &=  ~THREAD_FLAG_PARENT;
        retcount--;
        if(retcount == 0)
            hal_spin_unlock(&forkLock);
        return 0;
    }

    panic("t_prepare_fork no fork flag at all!");
}


static int old_sp;
static phantom_thread_t *parent;
static phantom_thread_t *child;

void phantom_thread_in_interrupt_fork()
{
    SHOW_INFO0( 10, "ifork in...");
    assert( forkLock.lock != 0 );
    hal_spin_lock(&schedlock);

    child = get_thread(GET_CURRENT_THREAD()->child_tid);
    parent = GET_CURRENT_THREAD();


//#warning cli
    // Save to me, switch to me
    SHOW_INFO0( 10, "ifork save...");
    phantom_switch_context(parent, parent, &schedlock );
    SHOW_INFO0( 10, "ifork saved...");
    // (OLD) phantom_switch_context() below returns here in old thread!!

    if(!(parent->thread_flags & THREAD_FLAG_PREFORK))
    {
        set_esp(old_sp);
        SHOW_INFO0( 10, "ifork in old...");
        // Second return. We're in old tread and done with fork;
        //GET_CURRENT_THREAD() = parent;
        SET_CURRENT_THREAD(parent);

        // Let child be elegible to run
        child->sleep_flags &= ~THREAD_SLEEP_LOCKED;
        t_enqueue_runq(child);

        return;
    }
    SHOW_INFO0( 10, "ifork cont...");

    parent->thread_flags &= ~THREAD_FLAG_PREFORK;
    //GET_CURRENT_THREAD() = child;
    SET_CURRENT_THREAD(child);

    // Now switch stack and copy some 512 bytes there to make sure
    // new one will have some place to return to

    int cp_size = 512;
    void *from = (void*) (old_sp = get_esp());
    void *to = (child->stack) - cp_size - 1;

    SHOW_INFO0( 10, "ifork memmove...");
    memmove(to, from, cp_size);

    //printf("set ESP...\n");
    SHOW_INFO0( 10, "set ESP...");
    set_esp((int)to);

//#warning sti
    // Save to new, switch to me -- WILL RETURN TO (X)
//printf("ifork GO...\n");
    phantom_switch_context(child, parent, &schedlock );
    // (NEW) phantom_switch_context() below returns here in new thread!!

//printf("ifork in NEW...\n");


}



#endif

