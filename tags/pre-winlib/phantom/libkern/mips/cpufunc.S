#include <mips/asm.h>
#include <mips/cp0_regs.h>

.text
.align	0


// This one complies to generic interface
LEAF(arch_get_fault_address)
    mfc0    v0, CP0_BADVADDR
    jr ra
    nop // delay slot
END(arch_get_fault_address)


// cpuid
//    mfc0	t0, CP0_PRID


