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
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10


#include <malloc.h>
#include <string.h>
#include <phantom_libc.h>
#include <kernel/page.h>
#include <kernel/smp.h>
#include <kernel/init.h>
#include <thread_private.h>

#if ARCH_mips
#include <kernel/vm.h>
#endif

static int find_tid(phantom_thread_t *);
static void common_thread_init(phantom_thread_t *t, int stacksize );

static hal_spinlock_t tid_lock; //init?


#define DEF_STACK_SIZE (128*1024)
//#define DEF_STACK_SIZE (256*1024)




//#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

phantom_thread_t *
phantom_create_thread( void (*func)(void *), void *arg, int flags )
{
    assert(threads_inited);
    assert( ! (flags & ~CREATION_POSSIBLE_FLAGS) );

#if NEW_SNAP_SYNC
    // No thread starts in snap, sorry
    snap_lock();
#endif

    SHOW_FLOW( 7, "flags = %b", flags, "\020\1USER\2VM\3JIT\4NATIVE\5KERNEL\6?PF\7?PA\10?CH\11TIMEOUT\12UNDEAD\13NOSCHED" );
    phantom_thread_t *t = calloc(1, sizeof(phantom_thread_t));
    //phantom_thread_t *t = calloc_aligned(1, sizeof(phantom_thread_t),16); // align at 16 bytes for ia32 fxsave

    // Can't be run yet
    t->sleep_flags = THREAD_SLEEP_LOCKED; 

    t->tid = find_tid(t);
    SHOW_FLOW( 7, "tid = %d", t->tid );

    // inherit ctty
    t->ctty = GET_CURRENT_THREAD()->ctty;

    common_thread_init(t, DEF_STACK_SIZE );
    //t->priority = THREAD_PRIO_NORM;

    SHOW_FLOW( 7, "cpu = %d", t->cpu_id );


    t->start_func_arg = arg;
    t->start_func = func;

    phantom_thread_state_init(t);
    SHOW_FLOW0( 7, "phantom_thread_state_init done" );

    t->thread_flags |= flags;
    // Let it be elegible to run
    t->sleep_flags &= ~THREAD_SLEEP_LOCKED;

    t_enqueue_runq(t);
    SHOW_FLOW0( 7, "on run q" );

#if NEW_SNAP_SYNC
    snap_unlock();
#endif

    return t;

}

//#pragma GCC diagnostic pop


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



static void
kernel_thread_starter(void *func)
{
    void (*thread)(void) = (void (*)(void))func;
    thread();

    panic("some kernel thread is dead");
}


void //*
hal_start_kernel_thread(void (*thread)(void))
{
    //return 
    phantom_create_thread( kernel_thread_starter, thread, THREAD_FLAG_KERNEL );
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

    common_thread_init(t, DEF_STACK_SIZE );

    t->start_func_arg = 0;
    t->start_func = 0;

    phantom_thread_state_init(t);

    t->thread_flags |= THREAD_FLAG_UNDEAD;

    // Let it be elegible to run
    t->sleep_flags &= ~THREAD_SLEEP_LOCKED;

    //GET_CURRENT_THREAD() = t;
    SET_CURRENT_THREAD(t);

    t_current_set_name("Main");

}

// Called per each CPU except for boot one.
void
phantom_import_cpu_thread(int ncpu)
{
    assert(threads_inited);

    // No malloc on new CPU before thread is imported! Malloc has mutex!
    physaddr_t pa; // unused
    phantom_thread_t *t; // = calloc(1, sizeof(phantom_thread_t));
    hal_pv_alloc( &pa, (void **)&t, sizeof(phantom_thread_t) );
    memset( t, 0, sizeof(phantom_thread_t) );

    // Can't be run yet
    t->sleep_flags = THREAD_SLEEP_LOCKED; 

    t->tid = find_tid(t);

    t->start_func_arg = 0;
    t->start_func = 0;

    common_thread_init(t, DEF_STACK_SIZE );
    phantom_thread_state_init(t);

    t->thread_flags |= THREAD_FLAG_UNDEAD;
    t->thread_flags |= THREAD_FLAG_NOSCHEDULE;

    // Let it be elegible to run
    t->sleep_flags &= ~THREAD_SLEEP_LOCKED;

    //GET_CURRENT_THREAD() = t;
    SET_CURRENT_THREAD(t);

    char *name = calloc(1, 20);
    snprintf( name, 20, "CPU %d idle", ncpu );
    t_current_set_name(name);
    free(name);

    t->priority = THREAD_PRIO_IDLE;

    GET_IDLEST_THREAD() = t;
}














static void common_thread_init(phantom_thread_t *t, int stacksize )
{
    //t->thread_flags = 0;
    t->priority = THREAD_PRIO_NORM;

    t->cpu_id = GET_CPU_ID();

    if( 0 == t->ctty ) t->ctty = wtty_init();

    // malloc uses mutex, so we have to use physalloc which is protected with spinlocks
    physaddr_t pa;

    t->stack_size = stacksize;
    //t->stack = calloc( 1, stacksize );
    hal_pv_alloc( &pa, &(t->stack), stacksize+PAGE_SIZE );
    hal_page_control( pa, t->stack, page_unmap, page_noaccess ); // poor man's guard page - TODO support in page fault
    t->stack_pa = pa;

    SHOW_FLOW( 5, "main stk va %p pa %p", t->stack, (void *)pa );

    //assert(t->stack != 0);

    t->kstack_size = stacksize;
    //t->kstack = calloc( 1, stacksize );
    hal_pv_alloc( &pa, &(t->kstack), stacksize+PAGE_SIZE );
    hal_page_control( pa, t->kstack, page_unmap, page_noaccess ); // poor man's guard page - TODO support in page fault
    t->kstack_pa = pa;
    SHOW_FLOW( 5, "kern stk va %p pa %p", t->kstack, (void *)pa );
#if ARCH_mips
    // On mips we need unmapped kernel stack for mapping on MIPS is
    // done with exceptions too and unmapped stack is fault forever.
    // We achieve this by setting stack virtual address to its
    // physical address | 0x8000000 - this virt mem area is direct
    // mapped to physmem at 0
    assert( (addr_t)phystokv(t->kstack_pa) > 0x80000000 );
    assert( (addr_t)phystokv(t->kstack_pa) < 0xC0000000 );
    t->kstack_top = phystokv(t->kstack_pa) +t->kstack_size-4; // Why -4?
#else
    t->kstack_top = t->kstack+t->kstack_size-4; // Why -4?
#endif

    //assert(t->kstack != 0);

    t->owner = 0;
    //t->u = 0;
    t->pid = NO_PID;
    t->thread_flags = 0;;

    t->waitcond = 0;

    hal_spin_init( &(t->waitlock));

    queue_init(&(t->chain));
    queue_init(&(t->runq_chain));

    t->sw_unlock = 0;

    t->preemption_disabled = 0;

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

tid_t get_next_tid(tid_t tid, phantom_thread_t *out)
{
    tid_t ret = tid+1;
    int ie = hal_save_cli();
    hal_spin_lock( &tid_lock );

    while( phantom_kernel_threads[ret] == 0 )
    {
        ret++;

        if(ret >= MAX_THREADS)
        {
            ret = -1;
            goto finish;
        }
    }

    if(out)
        *out = *phantom_kernel_threads[ret];

finish:
    hal_spin_unlock( &tid_lock );
    if(ie) hal_sti();

    return ret;
}


// Called from machdep thread start asm code

void
phantom_thread_c_starter(void)
{
    void (*func)(void *);
    void *arg;
    phantom_thread_t *t;

    t = GET_CURRENT_THREAD();
    arg = t->start_func_arg;
    func = t->start_func;

    // Thread switch locked it before switching into us, we have to unlock
    hal_spin_unlock(&schedlock);

#if DEBUG
    printf("---- !! phantom_thread_c_starter !! ---\n");
#endif
    t->cpu_id = GET_CPU_ID();

    arch_float_init();

    // We're first time running here, set arch specific things up
    // NB!! BEFORE enablings interrupts!
    arch_adjust_after_thread_switch(t);


    hal_sti(); // Make sure new thread is started with interrupts on

#if 0 // usermode loader does it himself
    if( THREAD_FLAG_USER & t->thread_flags )
    {
        //switch_to_user_mode();
    }
#endif


    func(arg);
    t_kill_thread( t->tid );
    panic("thread %d returned from t_kill_thread", t->tid );
}



