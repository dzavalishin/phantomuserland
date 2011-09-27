#include <ia32/eflags.h>


void hal_wait_for_interrupt(void)
{
    __asm __volatile("sti");
    __asm __volatile("hlt" : : );
}

__asm (".p2align 4;\n"); // for profiler to distingush funcs

void        hal_cli()
{
    __asm __volatile("cli" : : : "memory");
}

__asm (".p2align 4;\n"); // for profiler to distingush funcs

void        hal_sti()
{
    __asm __volatile("sti");
}

__asm (".p2align 4;\n"); // for profiler to distingush funcs

int hal_is_sti()
{
    int	ef;
    __asm __volatile("pushfl; popl %0" : "=r" (ef));
    return ef & EFL_IF;
}

__asm (".p2align 4;\n"); // for profiler to distingush funcs

int hal_save_cli()
{
    int e;

    e = hal_is_sti();
    __asm __volatile("cli" : : : "memory");
    return e;
}

__asm (".p2align 4;\n"); // for profiler to distingush funcs
