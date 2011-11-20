#ifdef ARCH_arm
/*
 * PL011 UART driver
 *
 * Copyright (C) 2009 B Labs Ltd.
 */

#define DEBUG_MSG_PREFIX "pl011"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/driver.h>
#include <kernel/arm/devid.h>
#include <hal.h>
#include <arm/memio.h>
#include <dev/mem/mem_pl011_uart.h>

/* Error status bits in receive status register */
#define PL011_FE		(1 << 0)
#define PL011_PE		(1 << 1)
#define PL011_BE		(1 << 2)
#define PL011_OE		(1 << 3)

/* Status bits in flag register */
#define PL011_TXFE		(1 << 7)
#define PL011_RXFF		(1 << 6)
#define PL011_TXFF		(1 << 5)
#define PL011_RXFE		(1 << 4)
#define PL011_BUSY		(1 << 3)
#define PL011_DCD		(1 << 2)
#define PL011_DSR		(1 << 1)
#define PL011_CTS		(1 << 0)



typedef struct {
    hal_sem_t           rx_sem;
    hal_sem_t           tx_sem;
} pl011;





#if 0
void uart_tx_char(unsigned long base, char c)
{
    unsigned int val = 0;

    do {
        val = R32(dev->iobase + PL011_UARTFR);
    } while (val & PL011_TXFF);  /* TX FIFO FULL */

    W32(c, dev->iobase + PL011_UARTDR);
}

char uart_rx_char(unsigned long base)
{
    unsigned int val = 0;

    do {
        val = R32(base + PL011_UARTFR);
    } while (val & PL011_RXFE); /* RX FIFO Empty */

    return (char)R32((base + PL011_UARTDR));
}

#endif

/*
 * Sets the baud rate in kbps. It is recommended to use
 * standard rates such as: 1200, 2400, 3600, 4800, 7200,
 * 9600, 14400, 19200, 28800, 38400, 57600 76800, 115200.
 */
void pl011_set_baudrate(addr_t base, unsigned int baud,
                        unsigned int clkrate)
{
    const unsigned int uartclk = 24000000;	/* 24Mhz clock fixed on pb926 */
    unsigned int val = 0, ipart = 0, fpart = 0;

    /* Use default pb926 rate if no rate is supplied */
    if (clkrate == 0)
        clkrate = uartclk;
    if (baud > 115200 || baud < 1200)
        baud = 38400;	/* Default rate. */

    /* 24000000 / (38400 * 16) */
    ipart = 39;

    W32(base + PL011_UARTIBRD, ipart);
    W32(base + PL011_UARTFBRD, fpart);

    /*
     * For the IBAUD and FBAUD to update, we need to
     * write to UARTLCR_H because the 3 registers are
     * actually part of a single register in hardware
     * which only updates by a write to UARTLCR_H
     */
    val = R32(base + PL011_UARTLCR_H);
    W32(base + PL011_UARTLCR_H, val);

}

static void uart_init(unsigned long uart_base)
{
    /* Initialise data register for 8 bit data read/writes */
    pl011_set_word_width(uart_base, 8);

    /*
     * Fifos are disabled because by default it is assumed the port
     * will be used as a user terminal, and in that case the typed
     * characters will only show up when fifos are flushed, rather than
     * when each character is typed. We avoid this by not using fifos.
     */
    pl011_disable_fifos(uart_base);

    /* Set default baud rate of 38400 */
    pl011_set_baudrate(uart_base, 115200, 24000000);

    /* Set default settings of 1 stop bit, no parity, no hw flow ctrl */
    pl011_set_stopbits(uart_base, 1);
    pl011_parity_disable(uart_base);

    /* Enable rx, tx, and uart chip */
    pl011_tx_enable(uart_base);
    pl011_rx_enable(uart_base);
    pl011_uart_enable(uart_base);
}












void pl011_interrupt( void *arg )
{
    phantom_device_t * dev = arg;
    pl011 *uart = (pl011 *)dev->drv_private;


    int irq = R32(dev->iobase + PL011_UARTRIS);

    if(irq & (PL011_RXIRQ|PL011_RXTIMEOUTIRQ))
    {
        W32( dev->iobase + PL011_UARTICR, PL011_RXIRQ|PL011_RXTIMEOUTIRQ );
        hal_sem_release( &uart->rx_sem );
    }

    if(irq & PL011_TXIRQ)
    {
        W32( dev->iobase + PL011_UARTICR, PL011_TXIRQ );
        hal_sem_release( &uart->tx_sem );
    }
}




static int pl011_read( struct phantom_device *dev, void *buf, int len)
{
    pl011 *uart = (pl011 *)dev->drv_private;
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

static int pl011_write(struct phantom_device *dev, const void *buf, int len)
{
    pl011 *uart = (pl011 *)dev->drv_private;
    const char *cp = buf;

    int done = 0;

    while( len > 0 )
    {

        while( PL011_TXFF & R32((dev->iobase + PL011_UARTFR)) )
            hal_sem_acquire( &uart->tx_sem );

        unsigned int val = *cp++;
        W32( dev->iobase + PL011_UARTDR, val );
        done++;
        len--;
    }

    return done;
}







static int seq_number = 0;


phantom_device_t * driver_pl011_uart_probe( int port, int irq, int stage )
{
    (void) stage;

#if ARCH_arm
    if( arm_id( port, 0x011, 0xB105F00D, 1 ) )
        SHOW_ERROR0( 0, "id failed" );
        //return 0;
#endif

    uart_init( port );

    pl011 *uart = calloc( 1, sizeof(pl011) );

    hal_sem_init( &uart->rx_sem, "uart.rx" );
    hal_sem_init( &uart->tx_sem, "uart.tx" );

    phantom_device_t * dev = calloc(1, sizeof(phantom_device_t));
    dev->name = "pl011.uart";
    dev->seq_number = seq_number++;
    dev->drv_private = uart;

    dev->dops.read = pl011_read;
    dev->dops.write = pl011_write;

    dev->iobase = port;
    dev->irq = irq;
    //dev->iomem = ;
    //dev->iomemsize = ;


    if( hal_irq_alloc( irq, &pl011_interrupt, dev, HAL_IRQ_SHAREABLE ) )
    {
        SHOW_ERROR( 0, "IRQ %d is busy", irq );
        goto free2;
    }


    // TODO move to pl011_open, mask back on close
    W32(PL011_UARTIMSC, PL011_RXIRQ|PL011_RXTIMEOUTIRQ|PL011_TXIRQ );


    return dev;

free2:
    free( dev );
//free1:

    hal_sem_destroy( &uart->rx_sem );
    hal_sem_destroy( &uart->tx_sem );

    free( uart );

    return 0;
}





#endif

