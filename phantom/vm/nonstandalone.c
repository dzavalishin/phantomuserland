// This one is compiled with compiler's headers

#include <stdio.h>
#include <stdarg.h>

int hal_printf(const char *fmt, ...)
{
    va_list ap;
    int retval;

    va_start(ap, fmt);
    retval = vprintf(fmt, ap);
    va_end(ap);

    fflush(stdout);

    return (retval);
}
