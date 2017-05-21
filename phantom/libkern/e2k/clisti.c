//#include <ia32/eflags.h>

#include <hal.h>
#include <e2k/asm-generic/atomic-long.h>
#include <e2k/asm/machdep.h>
#include <e2k/asm/irqflags.h>

/*
void hal_wait_for_interrupt(void)
{
#error write me
//    __asm __volatile("sti");
//    __asm __volatile("hlt" : : );
}
*/

__asm (".p2align 4;\n"); // for profiler to distingush funcs

void        hal_cli()
{
	e2k_cli();
}

__asm (".p2align 4;\n"); // for profiler to distingush funcs

void        hal_sti()
{
	e2k_sti();
}

__asm (".p2align 4;\n"); // for profiler to distingush funcs

int hal_is_sti()
{
//    int	ef;
//#error write me
//    __asm __volatile("pushfl; popl %0" : "=r" (ef));
//    return ef & EFL_IF;
	return !arch_irqs_disabled();
}

__asm (".p2align 4;\n"); // for profiler to distingush funcs

int hal_save_cli()
{
    int e;

    e = hal_is_sti();
	e2k_cli();
    return e;
}

__asm (".p2align 4;\n"); // for profiler to distingush funcs
