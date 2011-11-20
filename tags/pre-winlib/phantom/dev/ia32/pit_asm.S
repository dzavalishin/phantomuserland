#include <ia32/asm.h>

// Waste 10 microseconds.

ENTRY(tenmicrosec)
    movl	EXT(ten_microsec_sleep_loop_count),%ecx		// cycle count for 10 microsecond loop
    tenmicroloop:
    loop	tenmicroloop
    ret
