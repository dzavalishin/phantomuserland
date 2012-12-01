/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * arm VIC - PL192 (and, maybe, 190)
 *
**/

#define DEBUG_MSG_PREFIX "pl192"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <arm/memio.h>


/* TODO irq chip gen interface 
 *
 * static struct irq_chip vic_chip = {
 *           .name   = "VIC",
 *          .ack    = vic_mask_irq,
 *          .mask   = vic_mask_irq,
 *          .unmask = vic_unmask_irq,
 *  };
 *
 *
**/


static int seq_number = 0;
phantom_device_t * driver_pl192_probe( int port, int irq, int stage )
{
    (void) irq;
    (void) stage;

    if(seq_number)
    {
        SHOW_ERROR0( 0, "Just one" );
        return 0;
    }

    SHOW_FLOW( 1, "probe @ %x", port );

    phantom_device_t * dev = malloc(sizeof(phantom_device_t));

    dev->iomem = port;
    dev->iomemsize = 0;
    dev->iobase = port;
    dev->irq = 0;

    if( check_pl192_sanity(dev->iomem) )
        goto free;

    dev->name = DEBUG_MSG_PREFIX;
    dev->seq_number = seq_number++;

    //pl192_t *es = calloc(1,sizeof(pl192_t));
    //assert(es);
    dev->drv_private = 0;

    if( init_pl192(dev) )
        goto free1;


    return dev;
free1:
    //free(es);

free:
    free(dev);
    return 0;
}


static errno_t check_reg(int ioa, u_int32_t value)
{
    u_int32_t r = R32(ioa);

    if( r != value )
    {
        SHOW_ERROR( 0, "Reg %x != %x", ioa, value );
        return ENXIO;
    }

    return 0;
}


static errno_t check_pl192_sanity(int iobase)
{
    u_int32_t id = R32(iobase+VIC_REG_HID0);

    if( (id & 0xFF) != 0x192 )
    {
        SHOW_ERROR( 0, "Part number is %x", id & 0xFF );
        return ENXIO;
    }

    SHOW_INFO( 1, "Id is %x", id );

    check_reg(iobase+VIC_REG_PCELLID0, 0x0D );
    check_reg(iobase+VIC_REG_PCELLID1, 0xF0 );
    check_reg(iobase+VIC_REG_PCELLID2, 0x05 );
    check_reg(iobase+VIC_REG_PCELLID3, 0xB1 );

    return 0;
}


static void pl192_mask_irq(phantom_device_t *dev, unsigned int irq)
{
    irq &= 31;
    W32( dev->iobase + VIC_REG_INTENCLEAR, 1 << irq);
}

static void pl192_unmask_irq(phantom_device_t *dev, unsigned int irq)
{
    irq &= 31;
    W32( dev->iobase + VIC_REG_INTENABLE, 1 << irq);
}


static void pl192_begin_interrupt(phantom_device_t *dev)
{
    u_int32_t isr_addr = R32( dev->iobase + VIC_REG_VECTOR_ADDR );
    // ignored, this read is required to ack to hw that interrupt
    // is being serviced
    (void) isr_addr;
}

static void pl192_end_interrupt(phantom_device_t *dev)
{
    // EIO
    W32( dev->iobase + VIC_REG_VECTOR_ADDR, 0 );
}















// ARM instructions!
extern char int_entry_table[];
//static u_int32 vector[32];

static void init_pl192((phantom_device_t *dev)
{
    // Undocumented, got from Linux drv
    W32( dev->iobase + VIC_ITCR, 0 );

    // Usual things
    W32( dev->iobase + VIC_REG_INTSELECT, 0 ); 		// All are irq, no fiq
    W32( dev->iobase + VIC_REG_INTENABLE, 0);  		// In fact, ahs no effect on 192
    W32( dev->iobase + VIC_REG_INTENCLEAR, ~0 );        // That is it, really

    W32( dev->iobase + VIC_REG_IRQ_STATUS, 0 );
    W32( dev->iobase + VIC_REG_FIQ_STATUS, 0 );

    W32( dev->iobase + VIC_REG_SOFTINTCLEAR, ~0 );

    W32( dev->iobase + VIC_REG_PROTECTION, 1 );         // No usermode access

    W32( dev->iobase + VIC_REG_WPRIOMASK, 0 );          // No priorities are masked (low 16 bits, bit per prio)

    int i;

    for (i = 0; i < 32; i++)
    {
        // 4 instructions, 4 bytes per instr
        W32( dev->iobase + VIC_REG_VECTADDR0 + i, int_entry_table+(i*4*4) );
        W32( dev->iobase + VIC_REG_VECTPRIORITY0, 7 ); // Default prio. Range is 0-F
    }

}



