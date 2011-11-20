/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * CPU identification headers.
 *
 *
**/

#ifndef CPUID_H
#define CPUID_H

#ifndef ARCH_ia32
#warning Intel32 code! Wrong arch?
#endif

#include <phantom_types.h>
#include <sys/cdefs.h>



/* Type of cache we are describing */
enum cache_type {
    CACHE_UNKNOWN,
    CACHE_ITLB, CACHE_DTLB, CACHE_UTLB,
    CACHE_DATA, CACHE_INSN, CACHE_UNIFIED,
    CACHE_TRACE, CACHE_PREFETCH
};

typedef struct cache_info {
    enum cache_type ci_type;
    int ci_size;
    int ci_level;
    int ci_assoc;
    int ci_linesize;
} cache_info_t;


typedef struct cache_static {
    u_int8_t key;
    cache_info_t val;
} cache_static_t;


#define CPU_FEATURE_COMMON 0
#define CPU_FEATURE_INTEL  1
#define CPU_FEATURE_AMD    2


typedef struct cpu_feature {
    int reg, bit;
    const char *sname;
    const char *lname;
} cpu_feature_t;


#define MAX_CPU_COUNT 8
typedef struct cpu_info {
    int ci_type;
} cpu_info_t;

extern cpu_info_t cpu_info[MAX_CPU_COUNT];

struct cpuid_regs {
    u_int32_t eax;
    u_int32_t ebx;
    u_int32_t ecx;
    u_int32_t edx;
} __packed;

typedef struct cpuid_regs cpuid_regs_t;



#endif // CPUID_H

