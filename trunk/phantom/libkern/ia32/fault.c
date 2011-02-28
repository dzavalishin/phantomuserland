#include <machdep.h>
#include <kernel/ia32/cpu.h>

addr_t arch_get_fault_address(void)
{
	return get_cr2();
}

