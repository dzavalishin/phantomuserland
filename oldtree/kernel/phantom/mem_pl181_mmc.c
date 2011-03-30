#ifdef ARCH_arm
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Arm PL031 RTC driver.
 *
**/


#define DEBUG_MSG_PREFIX "pl181.mmc"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/driver.h>
#include <kernel/arm/devid.h>
#include <hal.h>
#include <arm/memio.h>
#include "mem_pl181_mmc.h"




typedef struct {
    hal_spinlock_t      lock;
} pl181;




static void pl181_init( int port )
{
    W32( port + MMCIMASK0, 0);
    W32( port + MMCIMASK1, 0);
    W32( port + MMCICLEAR, 0xfff);


    int data = R32(port+MMCICLOCK);
    SHOW_INFO( 0, "Clock = 0x%X", data );
}

void pl181_cmd_interrupt( void *arg )
{
    phantom_device_t * dev = arg;
    pl181 *mmc = (pl181 *)dev->drv_private;

    u_int32_t status  = R32( dev->iobase + MMCISTATUS );
    SHOW_INFO( 0, "cmd IRQ Status = 0x%X", status );
}

void pl181_io_interrupt( void *arg )
{
    phantom_device_t * dev = arg;
    pl181 *mmc = (pl181 *)dev->drv_private;

    hal_spin_lock( &mmc->lock );

    u_int32_t status;
    do {
        status  = R32( dev->iobase + MMCISTATUS );
        SHOW_INFO( 0, "io IRQ Status = 0x%X", status );

    } while(status);

    hal_spin_unlock( &mmc->lock );
}


/*


static int pl181_read( struct phantom_device *dev, void *buf, int len)
{
    pl181 *mmc = (pl181 *)dev->drv_private;
    char *cp = buf;

    int done = 0;

    while( (len > 0) && (PL011_RXFE & R32(dev->iobase + PL011_UARTFR)) )
        hal_sem_acquire( &mmc->rx_sem );

    while( (len > 0) && !(PL011_RXFE & R32(dev->iobase + PL011_UARTFR)) )
    {
        *cp++ = 0xFF & R32(dev->iobase + PL011_UARTDR);

        done++;
        len--;
    }

    return done;

}

static int pl181_write(struct phantom_device *dev, const void *buf, int len)
{
    pl181 *mmc = (pl181 *)dev->drv_private;
    const char *cp = buf;

    int done = 0;

    while( len > 0 )
    {

        while( PL011_TXFF & R32((dev->iobase + PL011_UARTFR)) )
            hal_sem_acquire( &mmc->tx_sem );

        unsigned int val = *cp++;
        W32( val, dev->iobase + PL011_UARTDR );
        done++;
        len--;
    }

    return done;
}

*/





static int seq_number = 0;


phantom_device_t * driver_pl181_mmc_probe( int port, int irq, int stage )
{
    (void) stage;

#if ARCH_arm
    if( arm_id( port, 0x181, 0xB105F00D, 1 ) )
        //return 0;
        SHOW_ERROR0( 0, "id failed" );
#endif

    pl181_init( port );

    pl181 *mmc = calloc( 1, sizeof(pl181) );

    hal_spin_init( &mmc->lock );

    phantom_device_t * dev = calloc(1, sizeof(phantom_device_t));
    dev->name = "pl181.mmc";
    dev->seq_number = seq_number++;
    dev->drv_private = mmc;

    //dev->dops.read = pl181_read;
    //dev->dops.write = pl181_write;

    dev->iobase = port;
    dev->irq = irq;
    //dev->iomem = ;
    //dev->iomemsize = ;


    if( hal_irq_alloc( irq, &pl181_cmd_interrupt, dev, HAL_IRQ_SHAREABLE ) )
    {
        SHOW_ERROR( 0, "IRQ %d is busy", irq );
        goto free2;
    }

    // XXX hack - device uses 2 irqs and we suppose that they're ajacent
    if( hal_irq_alloc( irq+1, &pl181_io_interrupt, dev, HAL_IRQ_SHAREABLE ) )
    {
        SHOW_ERROR( 0, "IRQ %d is busy", irq );
        goto free2;
    }


    W32( port + MMCIMASK0, MCI_IRQENABLE);
    W32( port + MMCIMASK1, MCI_TXFIFOHALFEMPTYMASK|MCI_RXFIFOHALFFULLMASK);

    W32( port + MMCIPOWER, MCI_PWR_ON| 0x1C |MCI_OD|MCI_ROD ); // Max voltage, open-drain, resistor
    W32( port + MMCICLOCK, MCI_CLK_ENABLE|0xFF ); // Max divisor


    return dev;

free2:
    free( dev );
//free1:


    free( mmc );

    return 0;
}





#endif // ARCH_arm

