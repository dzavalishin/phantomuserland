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

// Read char from the debug console
int debug_console_getc(void);


void phantom_dump_windows_buf(char *bp, int len);

// Print to log file only, doubles in debug_ext.h
void lprintf(char const *format, ...);


#define	HD_COLUMN_MASK	0xff
#define	HD_DELIM_MASK	0xff00
#define	HD_OMIT_COUNT	(1 << 16)
#define	HD_OMIT_HEX	(1 << 17)
#define	HD_OMIT_CHARS	(1 << 18)

void hexdump(const void *ptr, int length, const char *hdr, int flags);


#endif // KERNEL_DEBUG_H
