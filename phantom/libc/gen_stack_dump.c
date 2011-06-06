#include <assert.h>
#include <threads.h>
#include <phantom_libc.h>  //printf

void stack_dump()
{
    printf("tid %d ", get_current_tid() );
    stack_dump_from(arch_get_frame_pointer());
}

