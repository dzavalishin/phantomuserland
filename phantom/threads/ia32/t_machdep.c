/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Machine dependent C code, ia32.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include <thread_private.h>
#include <phantom_libc.h>
#include <i386/seg.h>
#include <i386/eflags.h>
#include <i386/tss.h>

//#define FXDEBUG(a) a
#define FXDEBUG(a)

// asm code
void phantom_thread_trampoline(void);
//void phantom_thread_fp_init(void *temp, void *newstate );

/**
 * Initializes what is needed for thread to be ready to
 * switch to. Thread structure must be already filled.
 */
void phantom_thread_state_init(phantom_thread_t *t)
{
    t->cpu.esp = (int)(t->stack + t->stack_size);
    t->cpu.ebp = 0;
    t->cpu.eip = (int)phantom_thread_trampoline;
    //t->cpu.flags = 0;

    t->owner = 0;
    t->u = 0;
    t->thread_flags = 0;//THREAD_FLAG_KERNEL;

    t->waitcond = 0;

    hal_spin_init( &(t->waitlock));

    queue_init(&(t->chain));
    queue_init(&(t->runq_chain));

    t->sw_unlock = 0;

    t->preemption_disabled = 0;

    int *esp = (int *)(t->cpu.esp);

    // --- Will be popped by thread switch code ---
    // Simulate phantom_switch_context's three params
    STACK_PUSH(esp,0); // ridiculous?
    STACK_PUSH(esp,0);
    STACK_PUSH(esp,0);

    STACK_PUSH(esp,t->cpu.eip); // as if we called func

    STACK_PUSH(esp,t);// EBX
    STACK_PUSH(esp,t->start_func_arg);// EDI
    STACK_PUSH(esp,t->start_func);// ESI
    STACK_PUSH(esp,0);// CR2

    t->cpu.esp = (int)esp;
/*
	// God knows why it works just once for importing main thread,
	// then causes 'no fpu' exception 7 err 0
#if 0 && FXSAVE
    //char mystate[512];
    //char his_state[512];

	// We need two 16 bytes alinged buffers
	char state_buffer[512*2 + 16];  

	char  *my_state = state_buffer;

	while( ((unsigned)my_state) & 0xFu )
		my_state++;

	char *his_state = my_state+512;

    //phantom_thread_fp_init( mystate, t->cpu.fxstate );
//FXDEBUG(double x = 0.0 + 1.0);

    //asm volatile("fxsave %0" : : "m" (my_state));
    i386_fxsave(my_state);
//FXDEBUG(hexdump( &mystate, 512, "FXSTATE our", 0));
    asm volatile("finit " : : );

    //asm volatile("fxsave %0" : : "m" (his_state));
    i386_fxsave(his_state);
//FXDEBUG(hexdump( &mystate, 512, "FXSTATE init", 0));

    i386_fxrstor(my_state);
    //asm volatile("fxrstor %0" : : "m" (my_state));

#endif
*/
}

void switch_to_user_mode()
{
    //asm("ljmp %0, $0" : : "i" (USER_CS));

    /*

    Push:

    SS
    ESP
    EFLAGS
    CS
    EIP

    */

    // Set up a stack structure for switching to user mode.
    asm volatile("  \
                 sti; \
                 mov %1, %%ax; \
                 mov %%ax, %%ds; \
                 mov %%ax, %%es; \
                 mov %%ax, %%fs; \
                 mov %%ax, %%gs; \
                 \
                 mov %%esp, %%eax; \
                 pushl %1; \
                 pushl %%eax; \
                 pushf; \
                 pushl %0; \
                 push $1f; \
                 iret; \
                 1: \
                 "
                 : : "i" (USER_CS), "i" (USER_DS)
                );
}

void
phantom_thread_c_starter(void (*func)(void *), void *arg, phantom_thread_t *t)
{
    // Thread switch locked it befor switching into us, we have to unlock
    hal_spin_unlock(&schedlock);

#if DEBUG
    printf("---- !! phantom_thread_c_starter !! ---\n");
#endif

    // We're first time running here, set arch specific things up
    // NB!! BEFORE enablings ints!
    arch_adjust_after_thread_switch(t);;

    hal_sti(); // Make sure new thread is started with interrupts on

#if 1
    if( THREAD_FLAG_USER & t->thread_flags )
    {
        //switch_to_user_mode();
    }
#endif


    func(arg);
    t_kill_thread( t->tid );
    panic("thread %d returned from t_kill_thread", t->tid );

}

// Do what is required (arch specific) after switching to a new thread
void arch_adjust_after_thread_switch(phantom_thread_t *t)
{
    // TODO machdep, header
    extern struct i386_tss  tss;

    //phantom_thread_t *t = GET_CURRENT_THREAD();
    tss.esp0 = (addr_t)t->kstack_top;

    int ncpu = GET_CPU_ID();
    t->cpu_id = ncpu;

#warning not SMP compliant
    // NO! - kill that "Or else CPU doesn't take in account esp0 change :("
    phantom_load_main_tss();
    //phantom_load_cpu_tss(ncpu);
}

