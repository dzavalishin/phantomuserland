#include <ia32/asm.h>

Entry(bios_int_10)
    int		$0x10
    int		$0x21
Entry(bios_int_10_end)

