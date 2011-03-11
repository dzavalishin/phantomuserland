/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ARM debug console.
 *
**/

#include <kernel/debug.h>

#define SERIAL_BASE 0x16000000
#define SERIAL_FLAG_REGISTER 0x18
#define SERIAL_BUFFER_FULL (1 << 5)

void debug_console_putc(int c)
{
    /* Wait until the serial buffer is empty */
    while (*(volatile unsigned long*)(SERIAL_BASE + SERIAL_FLAG_REGISTER) 
                                       & (SERIAL_BUFFER_FULL));
    /* Put our character, c, into the serial buffer */
    *(volatile unsigned long*)SERIAL_BASE = c;
}
