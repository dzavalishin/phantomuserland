/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2017 Dmitry Zavalishin, dz@dz.ru
 *
 * Threads machine dependent C code, Elbrus 2000.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include <thread_private.h>
#include <phantom_libc.h>

//#include <ia32/seg.h>
//#include <ia32/eflags.h>
//#include <ia32/tss.h>
//#include <ia32/private.h>



//#define FXDEBUG(a)

// asm code
void phantom_thread_trampoline(void);
//void phantom_thread_fp_init(void *temp, void *newstate );

/**
 * Initializes what is needed for thread to be ready to
 * switch to. Thread structure must be already filled.
 */
void phantom_thread_state_init(phantom_thread_t *t)
{
    /*
    t->cpu.esp = (int)(t->stack + t->stack_size);
    t->cpu.ebp = 0;
    t->cpu.eip = (int)phantom_thread_trampoline;
    //t->cpu.flags = 0;


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
    */
}

void switch_to_user_mode()
{
    /**
     *    Push order:
     *
     *    SS
     *    ESP
     *    EFLAGS
     *    CS
     *    EIP
     *
    ** /

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
                */
    panic("user mode not implemented");
}


// Do what is required (arch specific) after switching to a new thread
void arch_adjust_after_thread_switch(phantom_thread_t *t)
{
    /*
    //#warning not SMP compliant
    // must find out which TSS is for our CPU and update it's esp0
    int ncpu = GET_CPU_ID();
    cpu_tss[ncpu].esp0 = (addr_t)t->kstack_top;
    */
}




void dump_thread_stack(phantom_thread_t *t)
{
    /*
    void *ebp = (void *)t->cpu.ebp;
    printf("Thread %d EIP 0x%08X, ", t->tid, t->cpu.eip);
    stack_dump_from(ebp);
    */
}
