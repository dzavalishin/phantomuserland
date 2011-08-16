/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * MIPS debug console.
 *
**/

#include <kernel/debug.h>

#warning must go to board

// TODO use memio.h
#define MEM(___a) *((volatile unsigned int *)(___a))

// this is for QEMU -M mips machine
#define SERIAL_BASE 0xB40003f8u

#define SERIAL_FLAG_REGISTER 0x18
#define SERIAL_TX_BUFFER_FULL (1 << 5)
#define SERIAL_RX_BUFFER_EMPTY (1 << 4)

static void do_putc(int c)
{
    /* Wait until the serial buffer is empty */
    while (*(volatile unsigned long*)(SERIAL_BASE + SERIAL_FLAG_REGISTER) 
                                       & (SERIAL_TX_BUFFER_FULL));
    /* Put our character, c, into the serial buffer */
    *(volatile unsigned long*)SERIAL_BASE = c;
}

void debug_console_putc(int c)
{
    if(c=='\n')
        do_putc('\r');
    do_putc(c);
}

int debug_console_getc(void)
{
    char c;

    // Wait until the serial RX buffer is not empty
    while (MEM(SERIAL_BASE + SERIAL_FLAG_REGISTER) & (SERIAL_RX_BUFFER_EMPTY))
        ;
    c = 0xFF & MEM(SERIAL_BASE);

    return c;
}



void arch_debug_console_init(void)
{
    // TODO set 115200 on com port
}
