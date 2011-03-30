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


#define DEBUG_MSG_PREFIX "pl031.rtc"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/driver.h>
#include <kernel/arm/devid.h>
#include <hal.h>
#include <arm/memio.h>
//#include "mem_pl031_rtc.h"


#define RTCDR           0x00

#define RTCCR           0x0c

#define RTCIMSC         0x10

#define RTCICR          0x1C

/*
typedef struct {
    //hal_sem_t           rx_sem;
    //hal_sem_t           tx_sem;
} pl031;
*/



static void pl031_init( addr_t port )
{

    W32(port+RTCIMSC, 0); // No interrupts

    int data = R32(port+RTCDR);
    SHOW_INFO( 0, "RTC = 0x%X", data );
}

void pl031_interrupt( void *arg )
{
    phantom_device_t * dev = arg;
    //pl031 *uart = (pl031 *)dev->drv_private;

    W32( dev->iobase + RTCICR, 1 );
}


/*




static int pl031_read( struct phantom_device *dev, void *buf, int len)
{
    pl031 *uart = (pl031 *)dev->drv_private;
    char *cp = buf;

    int done = 0;

    while( (len > 0) && (PL011_RXFE & R32(dev->iobase + PL011_UARTFR)) )
        hal_sem_acquire( &uart->rx_sem );

    while( (len > 0) && !(PL011_RXFE & R32(dev->iobase + PL011_UARTFR)) )
    {
        *cp++ = 0xFF & R32(dev->iobase + PL011_UARTDR);

        done++;
        len--;
    }

    return done;

}

static int pl031_write(struct phantom_device *dev, const void *buf, int len)
{
    pl031 *uart = (pl031 *)dev->drv_private;
    const char *cp = buf;

    int done = 0;

    while( len > 0 )
    {

        while( PL011_TXFF & R32((dev->iobase + PL011_UARTFR)) )
            hal_sem_acquire( &uart->tx_sem );

        unsigned int val = *cp++;
        W32( val, dev->iobase + PL011_UARTDR );
        done++;
        len--;
    }

    return done;
}

*/





static int seq_number = 0;

// Bus is 24MHz, we need 8, so divide by 3. Must go to board definitions!
#define PL050_BUS_FREQ_DIVIDER 3

phantom_device_t * driver_pl031_rtc_probe( int port, int irq, int stage )
{
    (void) stage;

#if ARCH_arm
    if( arm_id( port, 0x031, 0xB105F00D, 1 ) )
        //return 0;
        SHOW_ERROR0( 0, "id failed" );
#endif

    pl031_init( port );

    //pl031 *uart = calloc( 1, sizeof(pl031) );


    phantom_device_t * dev = calloc(1, sizeof(phantom_device_t));
    dev->name = "pl031.rtc";
    dev->seq_number = seq_number++;
    dev->drv_private = 0; //uart;

    //dev->dops.read = pl031_read;
    //dev->dops.write = pl031_write;

    dev->iobase = port;
    dev->irq = irq;
    //dev->iomem = ;
    //dev->iomemsize = ;

/* on stage 0 it is not possible?
    if( hal_irq_alloc( irq, &pl031_interrupt, dev, HAL_IRQ_SHAREABLE ) )
    {
        SHOW_ERROR( 0, "IRQ %d is busy", irq );
        goto free2;
    }
*/



    return dev;

//free2:
    free( dev );
//free1:


    //free( uart );

    return 0;
}




#endif


