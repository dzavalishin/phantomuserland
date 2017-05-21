/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2017 Dmitry Zavalishin, dz@dz.ru
 *
 * Elbrus2000 serial console IO.
 *
**/

//#include <ia32/pio.h>
#include <kernel/debug.h>
#include <kernel/boot.h>
#include <kernel/init.h>

#include <asm/e2k_api.h>

//#include <asm/e2k_debug.h>
//#include <asm/e2k.h>
#include <boot_io.h>

// FIXME copied defines from e2k.h

#define	LMS_CONS_DATA_PORT	0x300UL	/* On READ  - data from keyboard      */
					/* On WRITE - data to debug ouput     */
					/* port (console/journal)             */

#define	LMS_CONS_STATUS_PORT	0x301UL	/* On READ  - data available on 0x300 */
					/* On WRITE - shift count   for 0x304 */



static inline unsigned int e2k_rom_debug_inl( u_int16_t port)
{
	return E2K_READ_MAS_W(PHYS_X86_IO_BASE + port, MAS_IOADDR);
}

static inline void e2k_rom_debug_outb( u_int16_t port, u_int8_t byte)
{
       E2K_WRITE_MAS_B(PHYS_X86_IO_BASE + port, byte, MAS_IOADDR);
}

static inline void e2k_rom_debug_putc(char c)
{
	while (e2k_rom_debug_inl(LMS_CONS_DATA_PORT));

	e2k_rom_debug_outb(LMS_CONS_DATA_PORT, c);
	e2k_rom_debug_outb(LMS_CONS_DATA_PORT, 0);
}




void arch_debug_console_init(void)
{
}

void debug_console_putc(int c)
{
	e2k_rom_debug_putc(c);
}

/*

#define WAIT_COM_OUT 1

#define PORT 0x3F8

int dbg_baud_rate = 115200;

void debug_console_putc(int c)
{
    if(bootflag_no_comcon)
        return;

#if WAIT_COM_OUT
    while(! (inb(PORT+5) & 0x20) )
        ;
#endif
    if( c == '\n' )
        outb(PORT, '\r' );
    outb(PORT, c);
}

void arch_debug_console_init(void)
{
    short divisor = 115200 / dbg_baud_rate;

    outb( PORT+3, 0x80 );	// set up to load divisor latch	
    outb( PORT, divisor & 0xf );		// LSB 
    outb( PORT+1, divisor >> 8 );		// MSB 
    outb( PORT+3, 3 );		// 8N1
}

*/