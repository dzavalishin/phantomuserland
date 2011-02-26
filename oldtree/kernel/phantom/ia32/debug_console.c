#include <i386/pio.h>
#include <kernel/debug.h>


#define WAIT_COM_OUT 1

#define PORT1 0x3F8

void debug_console_putc(int c)
{
#if WAIT_COM_OUT
    while(! (inb(PORT1+5) & 0x20) )
        ;
#endif
    if( c == '\n' )
        outb(PORT1, '\r' );
    outb(PORT1, c);
}

