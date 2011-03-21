#include <arm/asm.h>

ENTRY(arch_get_frame_pointer)
    mov r0, r11
    mov	pc, lr
