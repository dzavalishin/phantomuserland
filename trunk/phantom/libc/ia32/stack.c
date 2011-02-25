#include <assert.h>

void *arch_stack_get_start()
{
    void *ebp;
    asm volatile ("movl %%ebp,%0" : "=r" (ebp));
    return ebp;
}
