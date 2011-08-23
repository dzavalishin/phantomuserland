/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Machine dependent C code, mips.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/



#include <thread_private.h>
#include <phantom_libc.h>
#include <cpu_state.h>
#include <mips/cp0_regs.h>


// asm code
void phantom_thread_trampoline(void);




/**
 * Initializes what is needed for thread to be ready to
 * switch to. Thread structure must be already filled.
 */
void phantom_thread_state_init(phantom_thread_t *t)
{
    //u_int32_t my_cpsr = __get_cpsr();

    t->cpu.sp = (int)(t->stack + t->stack_size);
    t->cpu.gp = 0; // ?
    t->cpu.fp = 0;
    t->cpu.ra = (int)phantom_thread_trampoline;
    t->cpu.status = mips_read_cp0_status(); // copy ours
    t->cpu.status &= ~ST_IE; // Make sure interrupts are off in new thread


    int *sp = (int *)(t->cpu.sp);

    // NB - access SP in pairs of int32 - step is 8 bytes

    // Simulate prev func's stack frame, of all zeroes, for stack trace to stop here
    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);

    // Stack - simulate saved by phantom_switch_context regs s0-s7
    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);

    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);

    // Stack - simulate saved by phantom_switch_context hi, lo and placeholder
    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);

    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);


    // --- Will be popped by thread switch code ---

    // Now contents for "pop {r4-r12}" in context stwitch, 9 registers


    STACK_PUSH(sp,t);			// R6
    STACK_PUSH(sp,t->start_func_arg);	// R5
    STACK_PUSH(sp,t->start_func); 	// R4

    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);
    STACK_PUSH(sp,0);

    STACK_PUSH(sp,t);			// R6
    STACK_PUSH(sp,t->start_func_arg);	// R5
    STACK_PUSH(sp,t->start_func); 	// R4

    t->cpu.sp = (int)sp;

    // Top of stack word is used as exception entry count - see entry.S
    int *s_top = t->kstack_top;
    *s_top = 0;

/*
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

// Or else we'll die in exception before threads are init.
static char k0_emergncy_stack[1024];
addr_t  curr_thread_k0_stack_top = k0_emergncy_stack + sizeof(k0_emergncy_stack) - 4;

void arch_adjust_after_thread_switch(phantom_thread_t *t)
{
#if HAVE_SMP
#  error rewrite me
#endif
    // It is used in trap entry code to switch to correct stack if
    // trap is from userland. TODO not SMP compilant
    curr_thread_k0_stack_top = (addr_t)t->kstack_top;
}

void switch_to_user_mode()
{
    u_int32_t st = mips_read_cp0_status();
    st |= ST_UM;
    mips_write_cp0_status(st);
}


// TODO it seems pretty arch independent. Merge with ia32 and move to general code?
void
phantom_thread_c_starter(void (*func)(void *), void *arg, phantom_thread_t *t)
{
    t = GET_CURRENT_THREAD();
    arg = t->start_func_arg;
    func = t->start_func;

    //SET_CURRENT_THREAD(t);
    // Thread switch locked it before switching into us, we have to unlock
    hal_spin_unlock(&schedlock);

    // TODO all arch thread starters and context

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






void dump_thread_stack(phantom_thread_t *t)
{
    void *fp = (void *)t->cpu.fp;
    printf("Thread %d IP 0x%08X, ", t->tid, t->cpu.ra);
    stack_dump_from(fp);
}
