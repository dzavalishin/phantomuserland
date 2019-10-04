
#include <stdarg.h>



void panic(const char *fmt, ...)
{
    va_list ap;

    // CI: this word is being watched by CI scripts. Do not change -- or change CI appropriately
    printf("Panic: ");

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    exit(33);
}

