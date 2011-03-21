#include <assert.h>

void *arch_get_frame_pointer()
{
    void *ebp;
    asm volatile ("movl %%ebp,%0" : "=r" (ebp));
    return ebp;
}
