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
#define DEBUG_CONSOLE_SERIAL_BASE 0xB40003f8u

#define SERIAL_FLAG_REGISTER 5

#define SERIAL_TX_EMPTY 0x20
#define SERIAL_RX_READY 0x01

#if 0
static void debug_console_do_putc(int c)
{
#if 0
    /* Wait until the serial buffer is empty */
    while(!*(volatile unsigned long*)(DEBUG_CONSOLE_SERIAL_BASE + SERIAL_FLAG_REGISTER) 
                                       & SERIAL_TX_EMPTY)
        ;
#endif
    /* Put our character, c, into the serial buffer */
    *((volatile unsigned long*)DEBUG_CONSOLE_SERIAL_BASE) = c;
}
#else
void debug_console_do_putc(int c); // in asm, entry.S

#endif

void debug_console_putc(int c)
{
    if(c=='\n')
        debug_console_do_putc('\r');
    debug_console_do_putc(c);
}

int debug_console_getc(void)
{
    char c;

    // Wait until the serial RX buffer is not empty
    while(!MEM(DEBUG_CONSOLE_SERIAL_BASE + SERIAL_FLAG_REGISTER) & SERIAL_RX_READY)
        ;
    c = 0xFF & MEM(DEBUG_CONSOLE_SERIAL_BASE);

    return c;
}



void arch_debug_console_init(void)
{
    // TODO set 115200 on com port
}
