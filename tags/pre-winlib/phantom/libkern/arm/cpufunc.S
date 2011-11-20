#include <arm/asm.h>

.text
.align	0


// This one complies to generic interface
ENTRY(arch_get_fault_address)
    mrc p15, 0, r0, c6, c0, 0
    RET

// This one is arm specific
ENTRY(arm_get_fault_status)
    mrc p15, 0, r0, c5, c0, 0
    RET



ENTRY(__get_cpsr)
    mrs  r0, cpsr_all
    RET

ENTRY(__set_cpsr)
    msr  cpsr_all, r0
    RET
