#include <phantom_libc.h>
#include <phantom_assert.h>
#include <hal.h>

#include "interrupts.h"

#include <i386/trap.h>
#include <i386/pio.h>
#include <i386/isa/pic.h>
#include <queue.h>

#include "../misc.h"



struct handler_q
{
    void 			(*ihandler)(void *);
    void 		*	arg;
    int                 	is_shareable;

    queue_chain_t               chain;
};


static queue_head_t		heads[PIC_IRQ_COUNT];
static hal_spinlock_t    	irq_list_lock;


int    irq_nest = 		SOFT_IRQ_NOT_PENDING;



void init_irq_allocator(void)
{
    int i;
    for( i = 0; i < PIC_IRQ_COUNT; i++ )
        queue_init(&heads[i]);

    hal_spin_init(&irq_list_lock);
}




void def_soft_irq_handler(struct trap_state *ts)
{
    dump_ss(ts);
    panic("\nDefault softint handler called!\n");
}




static void def_irq_handler(struct trap_state *s)
{
    unsigned irq = s->err;

    if( irq >= PIC_IRQ_COUNT )
        panic("IRQ %d > max %d", irq, PIC_IRQ_COUNT-1 );

    if( queue_empty( &heads[irq] ) )
    {
        printf("\nNo handler for IRQ %d\n", irq);
        return;
    }

    struct handler_q *it;

    // NB. SMP safe. this CPU locks us out with cli, others will lock
    // (and unlock!) spinlock.

// TODO we can't use spinlocks here. lets hope this list is not modified
// after interrupts are enabled
    //hal_spin_lock(&irq_list_lock);
    queue_iterate(&heads[irq], it, struct handler_q *, chain)
    {
        if( it->ihandler == 0 )
            panic("IRQ %d func is zero", irq );
        it->ihandler( it->arg );
        hal_cli(); // In case handler enabled 'em
    }
    //hal_spin_unlock(&irq_list_lock);

}


void (*soft_irq_handler)(struct trap_state *) = def_soft_irq_handler;



// TODO in fact we don't need this junk. Call def_irq_handler
// directly!
void (*pic_irq_handlers[PIC_IRQ_COUNT])(struct trap_state *ts) = {
    def_irq_handler,	def_irq_handler,    def_irq_handler,	def_irq_handler,
    def_irq_handler,	def_irq_handler,    def_irq_handler,	def_irq_handler,
    def_irq_handler,	def_irq_handler,    def_irq_handler,	def_irq_handler,
    def_irq_handler,	def_irq_handler,    def_irq_handler,	def_irq_handler
};







// turned off while not ready
int hal_irq_alloc( int irq, void (*func)(void *arg), void *_arg, int is_shareable )
{
    if( irq < 0 && irq >= PIC_IRQ_COUNT )
        panic("IRQ %d > max %d", irq, PIC_IRQ_COUNT-1 );

    if( (!is_shareable) && !queue_empty( &heads[irq] ) )
    {
        printf("IRQ %d asked exculsive, but other user exist", irq);
        return -1;
    }

    if( is_shareable && !queue_empty( &heads[irq] ) && (!((struct handler_q *)queue_first( &heads[irq] ))->is_shareable) )
    {
        printf("IRQ %d asked shared, but already exclusive", irq);
        return -1;
    }


    struct handler_q *out = malloc(sizeof(struct handler_q));
    if(out == 0)
        return -1;

    out->ihandler = func;
    out->arg = _arg;
    out->is_shareable = is_shareable;

    int ie = hal_save_cli();
    hal_spin_lock(&irq_list_lock);
    // BUG Not SMP clean. Queue insert is not atomic, will break
    // if interrupt is executed on other CPU
    queue_enter(&heads[irq], out, struct handler_q *, chain);

    // Do it in spinlock to avoid races with disable_irq
    phantom_pic_enable_irq(irq);

    hal_spin_unlock(&irq_list_lock);
    if(ie) hal_sti();

    return 0;
}


// turned off while not ready
void hal_irq_free( int irq, void (*func)(void *arg), void *_arg )
{
    if( irq < 0 && irq >= PIC_IRQ_COUNT )
        panic("IRQ %d > max %d", irq, PIC_IRQ_COUNT-1 );


    struct handler_q *it;

    int ie = hal_save_cli();
    hal_spin_lock(&irq_list_lock);
    queue_iterate(&heads[irq], it, struct handler_q *, chain)
    {
        if( it->ihandler == func && it->arg == _arg )
        {
            queue_remove(&heads[irq], it, struct handler_q *, chain);


            if(queue_empty( &heads[irq] ))
            {
                // Do it in spinlock to avoid races with disable_irq
                phantom_pic_disable_irq(irq);
            }

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
    if( sirq < 0 && sirq >= SOFT_IRQ_COUNT )
        panic("soft IRQ %d > max %d", sirq, SOFT_IRQ_COUNT-1 );

    softirq_requests |= (1 << sirq);
    REQUEST_SOFT_IRQ();
}


void hal_enable_softirq() { ENABLE_SOFT_IRQ(); }
void hal_disable_softirq() { DISABLE_SOFT_IRQ(); }


/**
 *
 * Called at the end of HW interrupt processing, before iret, but
 * after all the PIC housekeeping is done.
 *
 * User routines are called with HW ins enabled ans SW ints disabled.
 * Note that task switcher reenables SW ints after context switch.
 *
**/

void hal_softirq_dispatcher(struct trap_state *ts)
{
    hal_cli();

    while(softirq_requests)
    {
        int sirq;
        for(sirq = 0; sirq < SOFT_IRQ_COUNT; sirq++)
        {
            if( !(softirq_requests & (1 << sirq)) )
                continue;
            softirq_requests &= ~(1 << sirq);

            hal_sti();

            softirqs[sirq].ihandler(softirqs[sirq].arg);

            hal_cli();

        }
    }

#if USE_SOFTIRQ_DISABLE
    assert(IS_SOFT_IRQ_DISABLED());
#endif

    hal_sti();
}


void
hal_PIC_interrupt_dispatcher(struct trap_state *ts, int mask)
{
    check_global_lock_entry_count();

    irq_nest++;
    def_irq_handler(ts);
    irq_nest--;

    check_global_lock_entry_count();
    // now unmask PIC before running softint

    if( ts->err > 7 )
    {
        // Slave
        outb( 0xa1, mask );
    }
    else
    {
        // Master
        outb( 0x21, mask );
    }


    if(irq_nest)
        return;

#if USE_SOFTIRQ_DISABLE

    // Now for soft IRQs
    irq_nest = SOFT_IRQ_DISABLED|SOFT_IRQ_NOT_PENDING;
    hal_softirq_dispatcher(ts);
    ENABLE_SOFT_IRQ();

#else
#warning do not select me

    // Now for soft IRQs
    //irq_nest = SOFT_IRQ_DISABLED|SOFT_IRQ_NOT_PENDING;
    irq_nest = SOFT_IRQ_NOT_PENDING;
    irq_nest++; // Prevent softirqs from reenter
    hal_softirq_dispatcher(ts);
    irq_nest--;
    //ENABLE_SOFT_IRQ();
#endif
}
