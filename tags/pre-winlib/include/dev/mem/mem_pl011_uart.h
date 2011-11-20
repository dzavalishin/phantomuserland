/*
 * PL011 UART Generic driver implementation.
 * Copyright Bahadir Balban (C) 2009
 */
#ifndef __PL011_H__
#define __PL011_H__

#include <arm/memio.h>


/* Register offsets */
#define PL011_UARTDR		0x00
#define PL011_UARTRSR		0x04
#define PL011_UARTECR		0x04
#define PL011_UARTFR		0x18
#define PL011_UARTILPR		0x20
#define PL011_UARTIBRD		0x24
#define PL011_UARTFBRD		0x28
#define PL011_UARTLCR_H		0x2C
#define PL011_UARTCR		0x30
#define PL011_UARTIFLS		0x34
#define PL011_UARTIMSC		0x38
#define PL011_UARTRIS		0x3C
#define PL011_UARTMIS		0x40
#define PL011_UARTICR		0x44
#define PL011_UARTDMACR		0x48

/* IRQ bits for each uart irq event */
#define PL011_RXIRQ		(1 << 4)
#define PL011_TXIRQ		(1 << 5)
#define PL011_RXTIMEOUTIRQ	(1 << 6)
#define PL011_FEIRQ		(1 << 7)
#define PL011_PEIRQ		(1 << 8)
#define PL011_BEIRQ		(1 << 9)
#define PL011_OEIRQ		(1 << 10)


#define _write32(v,a) W32(a,v)


static void pl011_set_baudrate(addr_t base, unsigned int baud,
                        unsigned int clkrate);

#define PL011_UARTEN	(1 << 0)
static inline void pl011_uart_enable(addr_t base)
{
    unsigned int val = 0;

    val = R32((base + PL011_UARTCR));
    val |= PL011_UARTEN;
    _write32(val, (base + PL011_UARTCR));

    return;
}

static inline void pl011_uart_disable(addr_t base)
{
    unsigned int val = 0;

    val = R32((base + PL011_UARTCR));
    val &= ~PL011_UARTEN;
    _write32(val, (base + PL011_UARTCR));

    return;
}

#define PL011_TXE	(1 << 8)
static inline void pl011_tx_enable(addr_t base)
{
    unsigned int val = 0;

    val = R32((base + PL011_UARTCR));
    val |= PL011_TXE;
    _write32(val, (base + PL011_UARTCR));
    return;
}

static inline void pl011_tx_disable(addr_t base)
{
    unsigned int val = 0;

    val =R32((base + PL011_UARTCR));
    val &= ~PL011_TXE;
    _write32(val, (base + PL011_UARTCR));
    return;
}

#define PL011_RXE	(1 << 9)
static inline void pl011_rx_enable(addr_t base)
{
    unsigned int val = 0;

    val = R32((base + PL011_UARTCR));
    val |= PL011_RXE;
    _write32(val, (base + PL011_UARTCR));
    return;
}

static inline void pl011_rx_disable(addr_t base)
{
    unsigned int val = 0;

    val = R32((base + PL011_UARTCR));
    val &= ~PL011_RXE;
    _write32(val, (base + PL011_UARTCR));
    return;
}

#define PL011_TWO_STOPBITS_SELECT	(1 << 3)
static inline void pl011_set_stopbits(addr_t base, int stopbits)
{
    unsigned int val = 0;

    val = R32((base + PL011_UARTLCR_H));

    if(stopbits == 2) {			/* Set to two bits */
        val |= PL011_TWO_STOPBITS_SELECT;
    } else {				/* Default is 1 */
        val &= ~PL011_TWO_STOPBITS_SELECT;
    }
    _write32(val, (base + PL011_UARTLCR_H));
    return;
}

#define PL011_PARITY_ENABLE	(1 << 1)
static inline void pl011_parity_enable(addr_t base)
{
    unsigned int val = 0;

    val = R32((base +PL011_UARTLCR_H));
    val |= PL011_PARITY_ENABLE;
    _write32(val, (base + PL011_UARTLCR_H));
    return;
}

static inline void pl011_parity_disable(addr_t base)
{
    unsigned int val = 0;

    val = R32((base + PL011_UARTLCR_H));
    val &= ~PL011_PARITY_ENABLE;
    _write32(val, (base + PL011_UARTLCR_H));
    return;
}

#define PL011_PARITY_EVEN	(1 << 2)
static inline void pl011_set_parity_even(addr_t base)
{
    unsigned int val = 0;

    val = R32((base + PL011_UARTLCR_H));
    val |= PL011_PARITY_EVEN;
    _write32(val, (base + PL011_UARTLCR_H));
    return;
}

static inline void pl011_set_parity_odd(addr_t base)
{
    unsigned int val = 0;

    val = R32((base + PL011_UARTLCR_H));
    val &= ~PL011_PARITY_EVEN;
    _write32(val, (base + PL011_UARTLCR_H));
    return;
}

#define	PL011_ENABLE_FIFOS	(1 << 4)
static inline void pl011_enable_fifos(addr_t base)
{
    unsigned int val = 0;

    val = R32((base + PL011_UARTLCR_H));
    val |= PL011_ENABLE_FIFOS;
    _write32(val, (base + PL011_UARTLCR_H));
    return;
}

static inline void pl011_disable_fifos(addr_t base)
{
    unsigned int val = 0;

    val = R32((base + PL011_UARTLCR_H));
    val &= ~PL011_ENABLE_FIFOS;
    _write32(val, (base + PL011_UARTLCR_H));
    return;
}

/* Sets the transfer word width for the data register. */
static inline void pl011_set_word_width(addr_t base, int size)
{
    unsigned int val = 0;
    if(size < 5 || size > 8)	/* Default is 8 */
        size = 8;

    /* Clear size field */
    val = R32((base + PL011_UARTLCR_H));
    val &= ~(0x3 << 5);
    _write32(val, (base + PL011_UARTLCR_H));

    /*
     * The formula is to write 5 less of size given:
     * 11 = 8 bits
     * 10 = 7 bits
     * 01 = 6 bits
     * 00 = 5 bits
     */
    val = R32((base + PL011_UARTLCR_H));
    val |= (size - 5) << 5;
    _write32(val, (base + PL011_UARTLCR_H));

    return;
}

/*
 * Defines at which level of fifo fullness an irq will be generated.
 * @xfer: tx fifo = 0, rx fifo = 1
 * @level: Generate irq if:
 * 0	rxfifo >= 1/8 full  	txfifo <= 1/8 full
 * 1	rxfifo >= 1/4 full  	txfifo <= 1/4 full
 * 2	rxfifo >= 1/2 full  	txfifo <= 1/2 full
 * 3	rxfifo >= 3/4 full	txfifo <= 3/4 full
 * 4	rxfifo >= 7/8 full	txfifo <= 7/8 full
 * 5-7	reserved		reserved
 */
static inline void pl011_set_irq_fifolevel(addr_t base, \
                                           unsigned int xfer, unsigned int level)
{
    if(xfer != 1 && xfer != 0)	/* Invalid fifo */
        return;
    if(level > 4)			/* Invalid level */
        return;

    _write32(level << (xfer * 3), (base + PL011_UARTIFLS));
    return;
}

#endif /* __PL011__UART__ */
