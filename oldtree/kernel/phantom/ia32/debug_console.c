#include <ia32/pio.h>
#include <kernel/debug.h>
#include <kernel/boot.h>
#include <kernel/init.h>


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

    outb( PORT+3, 0x80 );	/* set up to load divisor latch	*/
    outb( PORT, divisor & 0xf );		/* LSB */
    outb( PORT+1, divisor >> 8 );		/* MSB */
    outb( PORT+3, 3 );		/* 8N1 */
}
