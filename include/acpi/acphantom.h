#ifndef __ACPHANTOM_H__
#define __ACPHANTOM_H__

/*
 * ACPICA configuration
 */
#define ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_USE_DO_WHILE_0
#define ACPI_FLUSH_CPU_CACHE()
/*
 * This is needed since sem_timedwait does not appear to work properly
 * on cygwin (always hangs forever).
 */
#define ACPI_USE_ALTERNATE_TIMEOUT


#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#if defined(__ia64__) || defined(__x86_64__)
#define ACPI_MACHINE_WIDTH          64
#define COMPILER_DEPENDENT_INT64    long
#define COMPILER_DEPENDENT_UINT64   unsigned long
#else
#define ACPI_MACHINE_WIDTH          32
#define COMPILER_DEPENDENT_INT64    long long
#define COMPILER_DEPENDENT_UINT64   unsigned long long
#define ACPI_USE_NATIVE_DIVIDE
#endif

#ifndef __cdecl
#define __cdecl
#endif

#define ACPI_ACQUIRE_GLOBAL_LOCK(GLptr, Acq) if (GLptr) Acq=1; else Acq=0;
#define ACPI_RELEASE_GLOBAL_LOCK(GLptr, Pending) Pending = 1

/* On Cygwin, pthread_t is a pointer */

#define ACPI_CAST_PTHREAD_T(pthread) ((ACPI_THREAD_ID) ACPI_TO_INTEGER (pthread))

/* uses GCC */

#include "acpi/acgcc.h"

#endif /* __ACPHANTOM_H__ */
