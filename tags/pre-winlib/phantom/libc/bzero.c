// We have asm version for this arch
#ifndef ARCH_ia32

//#include <sys/cdefs.h>
//__FBSDID("$FreeBSD: src/lib/libc/string/bzero.c,v 1.2.32.1 2008/11/25 02:59:29 kensmith Exp $");

#define	BZERO
#include "memset.c"

#endif
