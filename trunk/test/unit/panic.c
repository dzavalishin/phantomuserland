
//#include <assert.h>
#include <stdarg.h>
//#include <hal.h>
//#include <phantom_libc.h>



void panic(const char *fmt, ...)
{

    printf("Panic: ");
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);

    exit(33);
}

