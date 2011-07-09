/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel debugger interface.
 *
 *
**/

#ifndef KERNEL_DEBUG_H
#define KERNEL_DEBUG_H

int dbg_init(void);
void kernel_debugger(void);


int dbg_add_command(void (*func)(int, char **), const char *name, const char *desc);

// Output copy of console output to some serial port or something
void debug_console_putc(int c);


void phantom_dump_windows_buf(char *bp, int len);


#endif // KERNEL_DEBUG_H