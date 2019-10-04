/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Architecture independent interrupts handling/dispatching
 *
**/

#define DEBUG_MSG_PREFIX "intr"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>
#include <phantom_assert.h>
#include <kernel/stats.h>
#include <kernel/profile.h>
#include <hal.h>

#include <kernel/interrupts.h>
#include <kernel/trap.h>
#include <kernel/board.h>
#include <queue.h>


struct handler_q
{
    void 			(*ihandler)(void *);
    void 		*	arg;

    //int 			(*shandler)(void *);
    //void 		*	sarg;

    int                 	is_shareable;

    queue_chain_t               chain;
};


static queue_head_t		heads[MAX_IRQ_COUNT];
static hal_spinlock_t    	irq_list_lock;


int    irq_nest = 		SOFT_IRQ_NOT_PENDING;



void init_irq_allocator(void)
{
    int i;
    for( i = 0; i < MAX_IRQ_COUNT; i++ )
        queue_init(&heads[i]);

    hal_spin_init(&irq_list_lock);
}





void def_soft_irq_handler(struct trap_state *ts)
{
//#ifdef ARCH_ia32
    dump_ss(ts);
//#endif
    panic("\nDefault softint handler called!\n");
}









void call_irq_handler(struct trap_state *s, unsigned irq)
{
    (void) s; // TODO? pass to int handler?

    if( irq >= MAX_IRQ_COUNT )
        panic("IRQ %d > max %d", irq, MAX_IRQ_COUNT-1 );

    if( queue_empty( &heads[irq] ) )
    {
        printf("\nNo handler for IRQ %d\n", irq);
        return;
    }

	profile_interrupt_enter();

#ifdef ARCH_ia32
    // #define RTC interrupt
    if( irq == 8 )
        profiler_register_interrupt_hit( s->eip );
#endif

    struct handler_q *it;

    // NB. SMP safe. this CPU locks us out with cli, others will lock
    // (and unlock!) spinlock.

// TODO we can't use spinlocks here. lets hope this list is not modified
    // after interrupts are enabled

    // TODO just mask off IRQ when modifying it's handlers list

    //hal_spin_lock(&irq_list_lock);
    queue_iterate(&heads[irq], it, struct handler_q *, chain)
    {
        if( it->ihandler == 0 )
            panic("IRQ %d func is zero", irq );
        it->ihandler( it->arg );
        hal_cli(); // In case handler enabled 'em
		// TODO assert cli?
    }
    //hal_spin_unlock(&irq_list_lock);

	profile_interrupt_leave( irq );
}










errno_t hal_irq_alloc( int irq, void (*func)(void *arg), void *_arg, int is_shareable )
{
    if( irq < 0 && irq >= MAX_IRQ_COUNT )
        panic("IRQ %d > max %d", irq, MAX_IRQ_COUNT-1 );

    if( (!is_shareable) && !queue_empty( &heads[irq] ) )
    {
        printf("IRQ %d asked exculsive, but other user exist", irq);
        return EMLINK;
    }

    if( is_shareable && !queue_empty( &heads[irq] ) && (!((struct handler_q *)queue_first( &heads[irq] ))->is_shareable) )
    {
        printf("IRQ %d asked shared, but already exclusive", irq);
        return EMLINK;
    }


    struct handler_q *out = malloc(sizeof(struct handler_q));
    if(out == 0)
        return ENOMEM;

    out->ihandler = func;
    out->arg = _arg;
    out->is_shareable = is_shareable;

    // mask off IRQ when modifying it's handlers list
    board_interrupt_disable(irq);

    int ie = hal_save_cli();
    hal_spin_lock(&irq_list_lock);
    // Queue insert is not atomic, will break
    // if interrupt is executed on other CPU.
    // That is why we disable irq on controller before doing this.
    queue_enter(&heads[irq], out, struct handler_q *, chain);

    // Do it in spinlock to avoid races with disable_irq
    board_interrupt_enable(irq);

    hal_spin_unlock(&irq_list_lock);
    if(ie) hal_sti();

    return 0;
}



void hal_irq_free( int irq, void (*func)(void *arg), void *_arg )
{
    if( irq < 0 && irq >= MAX_IRQ_COUNT )
        panic("IRQ %d > max %d", irq, MAX_IRQ_COUNT-1 );

    // Queue manipulationt is not atomic, will break
    // if interrupt is executed on other CPU.
    // That is why we disable irq on controller before doing this.
    board_interrupt_disable(irq);

    struct handler_q *it;

    int ie = hal_save_cli();
    hal_spin_lock(&irq_list_lock);
    queue_iterate(&heads[irq], it, struct handler_q *, chain)
    {
        if( it->ihandler == func && it->arg == _arg )
        {
            queue_remove(&heads[irq], it, struct handler_q *, chain);


            if(!queue_empty( &heads[irq] ))
                board_interrupt_enable(irq);

            // TODO free struct!

            hal_spin_unlock(&irq_list_lock);
            if(ie) hal_sti();
            return;
        }
    }
    hal_spin_unlock(&irq_list_lock);
    if(ie) hal_sti();

    panic("Attempted to free unallocated IRQ %d\n", irq );
}






// -----------------------------------------------------------------------
// Software IRQs
// -----------------------------------------------------------------------




static struct handler_q softirqs[SOFT_IRQ_COUNT];
static u_int32_t softirq_requests = 0;

void hal_set_softirq_handler( int sirq, void (*func)(void *), void *_arg )
{
    if( sirq < 0 && sirq >= SOFT_IRQ_COUNT )
        panic("soft IRQ %d > max %d", sirq, SOFT_IRQ_COUNT-1 );

    if(softirqs[sirq].ihandler)
        panic("Attempt to resetup softirq %d", sirq );

    softirqs[sirq].ihandler = func;
    softirqs[sirq].arg = _arg;
    softirqs[sirq].is_shareable = 0;
}


void hal_request_softirq( int sirq )
{
    if( (sirq < 0) && (sirq >= SOFT_IRQ_COUNT) )
        panic("soft IRQ %d > max %d", sirq, SOFT_IRQ_COUNT-1 );

    softirq_requests |= (1 << sirq);
    REQUEST_SOFT_IRQ();
}


void hal_enable_softirq() { ENABLE_SOFT_IRQ(); }
void hal_disable_softirq() { DISABLE_SOFT_IRQ(); }

// TODO free softirq?
static int next_softirq = 0;
int  					
hal_alloc_softirq(void)
{
	if( next_softirq >= SOFT_IRQ_THREADS )
		return -1;
	return next_softirq++;
}


/**
 *
 * Called at the end of HW interrupt processing, before iret, but
 * after all the interrupt controller housekeeping is done.
 *
 * User routines are called with HW ins enabled ans SW ints disabled.
 * Note that task switcher reenables SW ints after context switch.
 *
**/

void hal_softirq_dispatcher(struct trap_state *ts)
{
    (void) ts;

    hal_cli();

    while(softirq_requests)
    {
        int sirq;
        for(sirq = SOFT_IRQ_COUNT-1; sirq >= 0 ; sirq--)
        {
            if( !(softirq_requests & (1 << sirq)) )
                continue;

            softirq_requests &= ~(1 << sirq);

            //hal_sti(); // TODO ERROR we have to come to scheduler thread switch without turning interrupts on! We keep spinlock locked!

            softirqs[sirq].ihandler(softirqs[sirq].arg);
            STAT_INC_CNT(STAT_CNT_SOFTINT);

            hal_cli();

            //if( 0 == softirq_requests ) goto final; // Usually we have just one
            if( 0 == softirq_requests ) break; // Usually we have just one
        }
    }
//final:
#if USE_SOFTIRQ_DISABLE
    assert(IS_SOFT_IRQ_DISABLED());
#endif

    hal_sti();
}




// -----------------------------------------------------------------------
// general (arch indep) irq processing - can be replaced in arch
// -----------------------------------------------------------------------

#ifndef ARCH_ia32
void process_irq(struct trap_state *ts, int irq)
{
    ts->intno = irq;

    board_interrupt_disable(irq);

    irq_nest++;
    call_irq_handler( ts, irq );
    irq_nest--;

    check_global_lock_entry_count();

    // Must bring interrupt hw to EOI state also - softint can switch context
    board_interrupt_enable(irq); // TODO Wrong! Int handler might disable itself! Keep local mask.

    STAT_INC_CNT(STAT_CNT_INTERRUPT);

    if(irq_nest)
        return;

    // Now for soft IRQs
    irq_nest = SOFT_IRQ_DISABLED|SOFT_IRQ_NOT_PENDING;
    hal_softirq_dispatcher(ts);
    ENABLE_SOFT_IRQ();
}
#endif






















