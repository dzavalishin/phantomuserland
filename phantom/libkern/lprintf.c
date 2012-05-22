#include <phantom_libc.h>
#include <kernel/debug.h>

//void debug_console_putc(int c)

static void  log_putchar(int ch, void *arg)
{
    debug_console_putc(ch);
}


// Print to log file only
void lprintf(char const *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    kvprintf(fmt, log_putchar, 0, 10, ap);
    va_end(ap);
}
