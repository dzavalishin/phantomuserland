#include <kernel/init.h>

void arch_float_init(void)
{
    asm volatile ("fninit");
}

