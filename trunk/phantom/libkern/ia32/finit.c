#include <kernel/init.h>
#include <ia32/proc_reg.h>

void arch_float_init(void)
{
    asm volatile ("fninit");

    u_int32_t cr4 = get_cr4();

    cr4 |= CR4_FXSR|CR4_XMM;

    set_cr4(cr4);
}

