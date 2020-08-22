#ifdef ARCH_arm

/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2013 Dmitry Zavalishin, dz@dz.ru
 *
 * ARM raspberry PI (bcm2835) interrupts controller.
 *
**/



#define DEBUG_MSG_PREFIX "bcm2835intr"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_assert.h>

#include <arm/memio.h>

#include <kernel/debug.h>
#include <kernel/trap.h>
#include <kernel/interrupts.h>

#include "driver_arm_raspberry_interrupts.h"

static int arm_bcm2835_irq_dispatch(struct trap_state *ts);




static int low_irq_map[32] =
{
    // 0
    ARM_INT_TIMER,              ARM_INT_MAILBOX,
    ARM_INT_DOORBELL_0,         ARM_INT_DOORBELL_1,
    // 4
    ARM_INT_GPU_HALT_0,         ARM_INT_GPU_HALT_1,
    ARM_INT_ILLEGAL_ACC_1,      ARM_INT_ILLEGAL_ACC_0,

    // 8
    -1, -1, // no use - mark secondary regs pending

    // 10
    BCM2835_GPU_INT_START+7,    BCM2835_GPU_INT_START+9,

    // 12
    BCM2835_GPU_INT_START+10,   BCM2835_GPU_INT_START+18,
    BCM2835_GPU_INT_START+19,   BCM2835_GPU_INT_START+53,

    // 16
    BCM2835_GPU_INT_START+54,   BCM2835_GPU_INT_START+55,
    BCM2835_GPU_INT_START+56,   BCM2835_GPU_INT_START+57,

    // 20
    BCM2835_GPU_INT_START+62,	-1,

    -1, -1, -1, -1, // 22-25, unused
    -1, -1, -1, -1, // 26-29, unused
    -1, -1          // 30-31, unused

};


static int arm_bcm2835_irq_dispatch(struct trap_state *ts)
{
    u_int32_t   pend0;
    u_int32_t   irqs;

    int nirq = BCM2835_ARM_INT_START;

    while( (irqs = R32(BCM2835_INTR_IRQ_PENDING_BASIC)) != 0 )
    {
        pend0 = irqs;
        irqs &= ~0x300; // clear bits 8-9 used just in pend0
        while( irqs )
        {
            if( irqs & 0x1 )
            {
                int mapped_ino = low_irq_map[nirq];
                if(mapped_ino < 0) panic("impossible BCM2835 interrupt");
                process_irq(ts, mapped_ino);
            }

            irqs >>= 1;
            nirq++;
        }
    }


    nirq = BCM2835_GPU_INT_START;

    if( pend0 & 0x100 )
    {
        while( (irqs = R32(BCM2835_INTR_IRQ_PENDING_1)) != 0 )
        {
            while( irqs )
            {
                if( irqs & 0x1 )
                    process_irq(ts, nirq);

                irqs >>= 1;
                nirq++;
            }
        }
    }

    if( pend0 & 0x200 )
    {
        while( (irqs = R32(BCM2835_INTR_IRQ_PENDING_2)) != 0 )
        {
            while( irqs )
            {
                if( irqs & 0x1 )
                    process_irq(ts, nirq);

                irqs >>= 1;
                nirq++;
            }
        }
    }

    return 0; // We're ok
}




void arm_bcm2835_interrupt_enable(int irq)
{
    if( irq < 32 )
    {
        W32(BCM2835_INTR_IRQ_ENABLE_1,1<<irq);
    }
    else if( irq < 64 )
    {
        W32(BCM2835_INTR_IRQ_ENABLE_2,1<<(irq-32));
    }
    else if( irq < (32+64) )
    {
        W32(BCM2835_INTR_IRQ_ENABLE_BASIC,1<<(irq-(32+64)));
    }
    else
        panic("int no out of range");
}

void arm_bcm2835_interrupt_disable(int irq)
{
    if( irq < 32 )
    {
        W32(BCM2835_INTR_IRQ_DISABLE_1,1<<irq);
    }
    else if( irq < 64 )
    {
        W32(BCM2835_INTR_IRQ_DISABLE_2,1<<(irq-32));
    }
    else if( irq < (32+64) )
    {
        W32(BCM2835_INTR_IRQ_DISABLE_BASIC,1<<(irq-(32+64)) );
    }
    else
        panic("int no out of range");
}



void arm_bcm2835_interrupts_disable_all(void)
{
    // No way we ise FIQ
    W32( BCM2835_INTR_IRQ_FIQ, 0 );

    // Turn off all intrs
    W32( BCM2835_INTR_IRQ_DISABLE_BASIC, ~0 );
    W32( BCM2835_INTR_IRQ_DISABLE_1, ~0 );
    W32( BCM2835_INTR_IRQ_DISABLE_2, ~0 );
}



void arm_bcm2835_init_interrupts(void)
{
    arm_bcm2835_interrupts_disable_all();
    phantom_trap_handlers[T_IRQ] = arm_bcm2835_irq_dispatch;
}



#endif // ARCH_arm

