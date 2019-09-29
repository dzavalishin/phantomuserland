#include <arm/asm.h>

#warning actually used with arg == 1, ignores arg


ENTRY(arch_get_frame_pointer)
    mov r0, r11
    mov	pc, lr
