#include <ia32/asm.h>
__FBSDID("$FreeBSD: src/lib/libc/i386/string/memmove.S,v 1.7.36.1.6.1 2010/12/21 17:09:25 kensmith Exp $");

#define MEMMOVE
#include "asm_bcopy.S"
