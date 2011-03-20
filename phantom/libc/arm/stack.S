#include <arm/asm.h>

ENTRY(arch_stack_get_start)
    mov r0, r11
    mov	pc, lr
