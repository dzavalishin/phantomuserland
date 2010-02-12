
#include <mach/asm.h>

#include "SYS.h"

#define SYS_exit	0

SYSCALL(exit)
	ret		// Supposed to be impossible
