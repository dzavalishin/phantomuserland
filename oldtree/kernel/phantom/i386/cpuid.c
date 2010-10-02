/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * ia32 cpuid support
 *
**/

#define DEBUG_MSG_PREFIX "cpuid"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <cpuid.h>

#include <phantom_libc.h>
//#include <unistd.h>
#include <string.h>
#include <stdlib.h>
//#include <stdio.h>


char *viz32(u_int32_t);
char *humanize(u_int32_t);



const cache_static_t cd_array[] = {
    {0x01, {CACHE_ITLB, 4*1024, 0, 4, 32}},
    {0x02, {CACHE_ITLB, 4*1024*1024, 0, -1, 2}},
    {0x03, {CACHE_DTLB, 4*1024, 0, 4, 64}},
    {0x04, {CACHE_DTLB, 4*1024*1024, 0, 4, 8}},
    {0x05, {CACHE_DTLB, 4*1024*1024, 0, 4, 32}},

    {0x06, {CACHE_INSN, 8*1024, 1, 4, 32}},
    {0x08, {CACHE_INSN, 16*1024, 1, 4, 32}},
    {0x0A, {CACHE_DATA, 8*1024, 1, 2, 32}},
    {0x0C, {CACHE_DATA, 16*1024, 1, 4, 32}},
    {0x0E, {CACHE_UNIFIED, 24*1024, 1, 6, 64}},
    {0x22, {CACHE_UNIFIED, 512*1024, 3, 4, 64}},
    {0x23, {CACHE_UNIFIED, 1*1024*1024, 3, 8, 64}},
    {0x25, {CACHE_UNIFIED, 2*1024*1024, 3, 8, 64}},
    {0x29, {CACHE_UNIFIED, 4*1024*1024, 3, 8, 64}},
    {0x2C, {CACHE_DATA, 32*1024, 1, 8, 64}},
    {0x30, {CACHE_INSN, 32*1024, 1, 8, 64}},
    {0x39, {CACHE_UNIFIED, 128*1024, 2, 4, 64}},
    {0x3A, {CACHE_UNIFIED, 192*1024, 2, 6, 64}},
    {0x3B, {CACHE_UNIFIED, 128*1024, 2, 2, 64}},
    {0x3C, {CACHE_UNIFIED, 256*1024, 2, 4, 64}},
    {0x3D, {CACHE_UNIFIED, 384*1024, 2, 6, 64}},
    {0x3E, {CACHE_UNIFIED, 512*1024, 2, 4, 64}},

    {0x41, {CACHE_UNIFIED, 128*1024, 2, 4, 32}},
    {0x42, {CACHE_UNIFIED, 256*1024, 2, 4, 32}},
    {0x43, {CACHE_UNIFIED, 512*1024, 2, 4, 32}},
    {0x44, {CACHE_UNIFIED, 1*1024*1024, 2, 4, 32}},
    {0x45, {CACHE_UNIFIED, 2*1024*1024, 2, 4, 32}},
    {0x46, {CACHE_UNIFIED, 4*1024*1024, 3, 4, 64}},
    {0x47, {CACHE_UNIFIED, 8*1024*1024, 3, 8, 64}},
    {0x49, {CACHE_UNIFIED, 4*1024*1024, 2, 16, 64}},	/* XXX - 2 or 3 */
    {0x4A, {CACHE_UNIFIED, 6*1024*1024, 3, 12, 64}},
    {0x4B, {CACHE_UNIFIED, 8*1024*1024, 3, 16, 64}},
    {0x4C, {CACHE_UNIFIED, 12*1024*1024, 3, 12, 64}},
    {0x4D, {CACHE_UNIFIED, 16*1024*1024, 3, 16, 64}},
    {0x4E, {CACHE_UNIFIED, 6*1024*1024, 2, 24, 64}},

    {0x4F, {CACHE_ITLB, 4*1024, 0, -1, 32}},

    {0x50, {CACHE_ITLB, 4*1024, 0, -1, 64}},
    {0x51, {CACHE_ITLB, 4*1024, 0, -1, 128}},
    {0x52, {CACHE_ITLB, 4*1024, 0, -1, 256}},
    {0x56, {CACHE_DTLB, 4*1024*1024, 0, 4, 16}},
    {0x57, {CACHE_DTLB, 4*1024, 0, 4, 16}},
    {0x59, {CACHE_DTLB, 4*1024, 1, -1, 16}},
    {0x5B, {CACHE_DTLB, 4*1024, 0, -1, 64}},
    {0x5C, {CACHE_DTLB, 4*1024, 0, -1, 128}},
    {0x5D, {CACHE_DTLB, 4*1024, 0, -1, 256}},

    {0x60, {CACHE_DATA, 16*1024, 1, 8, 64}},
    {0x66, {CACHE_DATA, 8*1024, 1, 4, 64}},
    {0x67, {CACHE_DATA, 16*1024, 1, 4, 64}},
    {0x68, {CACHE_DATA, 32*1024, 1, 4, 64}},

    {0x70, {CACHE_TRACE, 12*1024, 0, 8, 0}},
    {0x71, {CACHE_TRACE, 16*1024, 0, 8, 0}},
    {0x72, {CACHE_TRACE, 32*1024, 0, 8, 0}},
    {0x73, {CACHE_TRACE, 64*1024, 0, 8, 0}},

    {0x78, {CACHE_UNIFIED, 1*1024*1024, 2, 4, 64}},
    {0x79, {CACHE_UNIFIED, 128*1024, 2, 8, 64}},
    {0x7A, {CACHE_UNIFIED, 256*1024, 2, 8, 64}},
    {0x7B, {CACHE_UNIFIED, 512*1024, 2, 8, 64}},
    {0x7C, {CACHE_UNIFIED, 1*1024*1024, 2, 8, 64}},
    {0x7D, {CACHE_UNIFIED, 2*1024*1024, 2, 8, 64}},
    {0x7F, {CACHE_UNIFIED, 512*1024, 2, 2, 64}},

    {0x80, {CACHE_UNIFIED, 512*1024, 2, 8, 64}},
    {0x82, {CACHE_UNIFIED, 256*1024, 2, 8, 32}},
    {0x83, {CACHE_UNIFIED, 512*1024, 2, 8, 32}},
    {0x84, {CACHE_UNIFIED, 1*1024*1024, 2, 8, 32}},
    {0x85, {CACHE_UNIFIED, 2*1024*1024, 2, 8, 32}},
    {0x86, {CACHE_UNIFIED, 512*1024, 2, 4, 64}},
    {0x87, {CACHE_UNIFIED, 1*1024*1024, 2, 8, 64}},

    {0xB0, {CACHE_ITLB, 4*1024, 0, 4, 128}},
    {0xB1, {CACHE_ITLB, 4*1024*1024, 0, 4, 4}},	/* XXX or 2*2M */
    {0xB3, {CACHE_DTLB, 4*1024, 0, 4, 128}},
    {0xB4, {CACHE_DTLB, 4*1024, 0, 4, 256}},
    {0xBA, {CACHE_DTLB, 4*1024, 0, 4, 64}},

    {0xC0, {CACHE_DTLB, 4*1024*1024, 0, 4, 8}},

    {0xF0, {CACHE_PREFETCH, 64, 0, 0, 0}},
    {0xF1, {CACHE_PREFETCH, 128, 0, 0, 0}},

    {0, {0, 0, 0, 0, 0}},
};



const cpu_feature_t cd_feature[] = {
    {3,  0, "FPU", "Floating Point Unit On-chip"},
    {3,  1, "VME", "Virtual Mode Extension"},
    {3,  2, "DE", "Debugging Extension"},
    {3,  3, "PSE", "Page Size Extension"},
    {3,  4, "TSC", "Time-Stamp Counter"},
    {3,  5, "MSR", "Model Specific Registers"},
    {3,  6, "PAE", "Physical Address Extension"},
    {3,  7, "MCE", "Machine Check Exception"},
    {3,  8, "CX8", "CMPXCHG8 Instruction Supported"},
    {3,  9, "APIC", "On-chip APIC Hardware Supported"},
    {3, 10, "RES1", "Reserved"},
    {3, 11, "SEP", "Fast System Call"},
    {3, 12, "MTRR", "Memory Type Range Registers"},
    {3, 13, "PGE", "Page Global Enable"},
    {3, 14, "MCA", "Machine Check Architecture"},
    {3, 15, "CMOV", "Conditional Move Instruction Supported"},
    {3, 16, "PAT", "Page Attribute Table"},
    {3, 17, "PSE36", "36-bit Page Size Extension"},
    {3, 18, "PSN", "Processor Serial Number is Enabled"},
    {3, 19, "CLFSH", "CLFLUSH Instruction Supported"},
    {3, 20, "RES2", "Reserved"},
    {3, 21, "DS", "Debug Store"},
    {3, 22, "ACPI", "Thermal Monitor & Software Controlled Clock"},
    {3, 23, "MMX", "IA32 MMX Supported"},
    {3, 24, "FXSR", "Fast Floating Point Save and Restore"},
    {3, 25, "SSE", "Streaming SIMD Extensions Supported"},
    {3, 26, "SSE2", "Steaming SIMD Extensions 2 Supported"},
    {3, 27, "SS", "Self-Snoop"},
    {3, 28, "HTT", "Hyper-Threading Supported"},
    {3, 29, "TM", "Thermal Monitor Supported"},
    {3, 30, "IA64", "IA64 Capabilities"},
    {3, 31, "PBE", "Pending Break Enable"},

    {2,  0, "SSE3", "Streaming SIMD Extensions 3 Supported"},
    {2,  1, "RES3", "Reserved"},
    {2,  2, "DTES64", "64-bit Debug Store"},
    {2,  3, "MONITOR", "Monitor/MWait Instructions Supported"},
    {2,  4, "DS-CPL", "CPL Qualified Debug Store"},
    {2,  5, "VMX", "Virtual Machine Extensions"},
    {2,  6, "SMX", "Safer Mode Extensions"},
    {2,  7, "EST", "Enhanced Intel SpeedStep"},
    {2,  8, "TM2", "Thermal Monitor 2"},
    {2,  9, "SSSE3", "Supplemental Streaming SIMD Extensions 3"},
    {2, 10, "CNXT-ID", "Context ID"},
    {2, 11, "RES4", "Reserved"},
    {2, 12, "RES5", "Reserved"},
    {2, 13, "CX16", "CMPXCHG16B Instruction Support"},
    {2, 14, "xTPR", "Send Task Priority Messages"},
    {2, 15, "PDCM", "Performance Capabilities MSR"},
    {2, 16, "RES6", "Reserved"},
    {2, 17, "RES7", "Reserved"},
    {2, 18, "DCA", "Direct Cache Access"},
    {2, 19, "SSE4.1", "Streaming SIMD Extensions 4.1"},
    {2, 20, "SSE4.2", "Streaming SIMD Extensions 4.2"},
    {2, 21, "RES8", "Reserved"},
    {2, 22, "RES9", "Reserved"},
    {2, 23, "POPCNT", "POPCNT Instruction Support"},
    {2, 24, "RES11", "Reserved"},
    {2, 25, "RES12", "Reserved"},
    {2, 26, "RES13", "Reserved"},
    {2, 27, "RES14", "Reserved"},
    {2, 28, "RES15", "Reserved"},
    {2, 29, "RES16", "Reserved"},
    {2, 30, "RES17", "Reserved"},
    {2, 31, "VIRT", "Virtually yours!"},

    {0, 0, NULL, NULL},
};


cpu_info_t cpu_info[MAX_CPU_COUNT];


const cache_info_t *
ci_find(u_int8_t key)
{
    const struct cache_static *p = &cd_array[0];
    while((p->key != 0) && p->key != key)
        p++;

    if(p->key == key)
        return &p->val;

    return NULL;
}

void
ci_print(const cache_info_t *ci)
{
    const char *type = NULL;

    switch(ci->ci_type){
    case CACHE_ITLB:		type = "instruction TLB"; break;
    case CACHE_DTLB:		type = "data TLB"; break;
    case CACHE_UTLB:		type = "unified TLB"; break;
    case CACHE_DATA:		type = "data cache"; break;
    case CACHE_INSN:		type = "instruction cache"; break;
    case CACHE_UNIFIED:		type = "unified cache"; break;
    case CACHE_TRACE:		type = "trace cache"; break;
    case CACHE_PREFETCH:	type = "Prefetch"; break;
    default:				type = "unknown cache"; break;
    }

    switch(ci->ci_type){
    case CACHE_ITLB:
    case CACHE_DTLB:
    case CACHE_UTLB:
        if (ci->ci_assoc < 0)
            printf( "%d-entry, %s page %s, fully associative\n",
                    ci->ci_linesize, humanize(ci->ci_size), type);
        else
            printf( "%d-entry, %s page %s, %d-way set associative\n",
                    ci->ci_linesize, humanize(ci->ci_size), type, ci->ci_assoc);
        /* Do TLB style */
        break;

    case CACHE_DATA:
    case CACHE_INSN:
    case CACHE_UNIFIED:
        /* Do memory style */
        printf( "%s %d-way, %d-byte line, level %d %s\n",
                humanize(ci->ci_size), ci->ci_assoc,
                ci->ci_linesize, ci->ci_level, type);
        break;

    case CACHE_TRACE:
        printf( "%s-uops %d-way %s\n",
                humanize(ci->ci_size), ci->ci_assoc, type);
        break;

    case CACHE_PREFETCH:
        printf( "%d-byte %s\n", ci->ci_size, type);
        /* Do misc style */
        break;

    default:
        /* Do unknown style */
        printf( "%d level-%d %s, %d-way, %d-byte line %s\n",
                ci->ci_size, ci->ci_level, type, ci->ci_assoc,
                ci->ci_linesize, type);
        break;
    }
}


cpuid_regs_t
cpuid(u_int32_t eax, u_int32_t ecx)
{
    struct cpuid_regs regs;

    __asm __volatile(
                     "cpuid"
                     : "=a" (regs.eax), "=b" (regs.ebx), "=c" (regs.ecx), "=d" (regs.edx)
                     : "0" (eax), "2" (ecx)
                    );

    return regs;
}

/*
 * Note, the code below works in 64-bit mode as well, magic...
 */
int
toggle_eflags(int bit)
{
    u_int32_t mask = 1 << bit;
    u_int32_t result = 0;

    __asm __volatile(
#ifndef __i386__
                     ".code32\n"
#endif
                     "pushfl;"
                     "popl	%%eax;"
                     "movl	%%eax, %%ecx;"
                     "xorl	%%ebx, %%eax;"
                     "pushl	%%eax;"
                     "popfl;pushfl;"
                     "popl	%%eax;"
                     "xorl	%%ecx, %%eax;"
                     "movl	$0, %%eax;"
                     "jz	1f;"
                     "pushl	%%ecx;"
                     "popfl;"
                     "movl	$1, %%eax;\n"
                     "1:\n"
#ifndef __i386__
                     ".code64\n"
#endif
                     : "=a" (result), "=b" (mask)
                     : "1" (mask)
                    );

    return result;
}

u_int32_t
check_leaf(u_int32_t eax)
{
    struct cpuid_regs regs;

    regs = cpuid(eax, 0);
    if ((regs.eax & 0xffff0000) == (eax & 0xffff0000))
        return regs.eax;
    return 0;
}

const char *
brand_leaf(u_int32_t eax, u_int32_t *mask)
{
    struct cpuid_regs r;
    char type_buf[13];
    const char *val = "Unknown Manufacturer";
    *mask = -1;

    r = cpuid(eax, 0);
    if (!eax) {
        memcpy(&type_buf[0], &r.ebx, 4);
        memcpy(&type_buf[4], &r.edx, 4);
        memcpy(&type_buf[8], &r.ecx, 4);
    } else {
        memcpy(&type_buf[0], &r.ebx, 4);
        memcpy(&type_buf[4], &r.ecx, 4);
        memcpy(&type_buf[8], &r.edx, 4);
    }
    type_buf[12] = 0;

    if(!strcmp(type_buf, "GenuineIntel")){
        *mask = 1<<0;
        val = "Intel";
    }else if(!strcmp(type_buf, "AuthenticAMD")){
        *mask = 1<<1;
        val = "AMD";
    }else if(!strcmp(type_buf, "GenuineTMx86")){
        *mask = 1<<2;
        val = "Transmeta";
    }else if(!strcmp(type_buf, "CyrixInstead")){
        *mask = 1<<3;
        val = "Cyrix";
    }else if(!strcmp(type_buf, "UMC UMC UMC ")){
        *mask = 1<<4;
        val = "UMC";
    }else if(!strcmp(type_buf, "NexGenDriven")){
        *mask = 1<<5;
        val = "NexGen";
    }else if(!strcmp(type_buf, "CentaurHauls")){
        *mask = 1<<6;
        val = "Centaur";
    }else if(!strcmp(type_buf, "RiseRiseRise")){
        *mask = 1<<7;
        val = "Rise";
    }else if(!strcmp(type_buf, "SiS SiS SiS ")){
        *mask = 1<<8;
        val = "SiS";
    }else if(!strcmp(type_buf, "Geode by NSC")){
        *mask = 1<<9;
        val = "National Semiconductor";
    }

    return val;
}

static int
check_eflags(int bit)
{
    u_int32_t flags = 0;

    __asm __volatile(
#ifndef __i386__
                     ".code32\n"
#endif
                     "pushfl;"
                     "popl	%%eax;"
#ifndef __i386__
                     ".code64\n"
#endif
                     : "=a" (flags)
                    );

    return (flags >> bit) & 1;
}

/*
 * Try to toggle the AC flag.  If no toggle, bad ju-ju, we are
 * at most a 386.  We should not even be here, but whatever,
 * we keep going with "minimal" support.
 */
int
flag_AC()
{
    return toggle_eflags(18);
}

int
flag_VIF()
{
    return check_eflags(19);
}

int
flag_VIP()
{
    return check_eflags(20);
}

int
flag_ID()
{
    return toggle_eflags(21);
}

char *
viz32(u_int32_t x)
{
    static char buf[sizeof(x) + 1];
    unsigned int i;

    for(i = 0; i < sizeof(x); i++){
        int c = x & 0xFF;
        if (c > 31 && c < 128 && isprint(c))
            buf[i] = c;
        else
            buf[i] = '.';
        x >>= 8;
    }

    return buf;
}

char *
humanize(u_int32_t x)
{
    static char buf[12];
    const char *suff[] = {"", "K", "M", "G", NULL};
    u_int32_t res;
    int i;

    for(i = 0; suff[i] != NULL; i++){
        res = x % 1024;
        if(res != 0) break;

        x /= 1024;
    }

    snprintf(buf, sizeof(buf), "%d%s", x, suff[i]);

    return buf;
}

u_int32_t
bitx(u_int32_t d, int h, int l)
{
    u_int32_t res;

    res = (((d) >> (l)) & ((1LU << ((h) - (l) + 1LU)) - 1LU));

    return res;
}

u_int32_t
bswap32(u_int32_t val)
{
    __asm __volatile(
                     "bswap %0"
                     : "=r" (val)
                     : "0" (val)
                    );

    return val;
}

void
dump_regs(struct cpuid_regs regs)
{
    printf("0x%.8x ", regs.eax);
    printf("0x%.8x ", regs.ebx);
    printf("0x%.8x ", regs.ecx);
    printf("0x%.8x ", regs.edx);

    printf("%s", viz32(regs.eax));
    printf("%s", viz32(regs.ebx));
    printf("%s", viz32(regs.ecx));
    printf("%s", viz32(regs.edx));

    printf("\n");
}

void
munge_brandstr(const char *p, char *q)
{
    while(*p && *p == ' ')
        p++;
    while(*p){
        while(*p == ' ' && *(p+1) == ' ')
            p++;
        *q++ = *p++;
    }
    *q = '\0';
}

/* 386* Humanized strings */
struct tfms_map {
    u_int32_t type;
    u_int32_t family;
    u_int32_t major;
    u_int32_t minor;
    const char *val;
} tfms_map[] = {
    {0, 3, 0, -1, "Intel 386 DX processor"},
    {2, 3, 0, -1, "Intel 386 SX processor"},
    {4, 3, 0, -1, "Intel 386 SL processor"},
    {4, 3, 1, -1, "Intel 386 SL processor"},
    {0, 3, 4, -1, "RapidCAD coprocessor"},

    {0, 0, 0, 0, NULL},
};

/* 486+ Humanized strings */
struct fmsid_map {
    u_int32_t key;
    u_int32_t mfg;
    const char *val;
} fmsid_map[] = {
    {0x00000400, 1, "486(tm) DX-25/33 processor"},
    {0x00000410, 1, "486(tm) DX-50 processor"},
    {0x00000410, 16, "U5D processor"},
    {0x00000420, 1, "486 SX processor"},
    {0x00000420, 16, "U5S processor"},
    {0x00000430, 3, "486 DX2 processor"},
    {0x00000440, 1, "486 SL processor"},
    {0x00000440, 8, "MediaGX processor"},
    {0x00000450, 1, "SX2 processor"},
    {0x00000470, 3, "DX2 WB processor"},
    {0x00000480, 3, "DX4 processor"},
    {0x00000490, 3, "DX4 WB processor"},
    {0x000004E0, 2, "Am5x86-WT processor"},
    {0x000004F0, 2, "Am5x86-WB processor"},
    {0x00001480, 1, "DX4 OverDrive processor"},
    {0x00000500, 1, "Pentium(R) 60/66 Rev. A processor"},
    {0x00000500, 2, "K5/SSA5 processor"},
    {0x00000500, 128, "mP6 processor"},
    {0x00000500, 256, "55x processor"},
    {0x00000510, 1, "Pentium(R) 60/66 processor"},
    {0x00000510, 2, "K5 processor"},
    {0x00000510, 32, "Nx586 processor"},
    {0x00000510, 128, "mP6 processor"},
    {0x00000520, 1, "Pentium 75-200 processor"},
    {0x00000520, 2, "K5 processor"},
    {0x00000520, 8, "6x86/6x86L processor"},
    {0x00000530, 1, "OverDrive PODP5V83 processor"},
    {0x00001510, 1, "Pentium OverDrive 60/66 processor"},
    {0x00001520, 1, "Pentium OverDrive 75-133 processor"},
    {0x00001530, 1, "Pentium OverDrive for Intel486 processor"},
    {0x00000530, 2, "K5 processor"},
    {0x00000540, 1, "Pentium 166/200/233 processor with MMX"},
    {0x00000540, 4, "Crusoe TM3x00/TM5x00 processor"},
    {0x00000540, 8, "MediaGX MMX processor"},
    {0x00000540, 64, "C6 processor"},
    {0x00000540, 512, "GX1/GXLV/GXm processor"},
    {0x00001540, 1, "Pentium 75-133 OverDrive processor with MMX"},
    {0x00000550, 512, "GX2 processor"},
    {0x00000560, 2, "K6 processor"},
    {0x00000570, 1, "Mobile Pentium 75-200 processor"},
    {0x00000570, 2, "K6 processor"},
    {0x00000580, 1, "Mobile Pentium MMX processor"},
    {0x00000580, 2, "K6-2 processor"},
    {0x00000580, 64, "C2 processor"},
    {0x00000590, 2, "K6-3 processor"},
    {0x00000590, 64, "C3 processor"},
    {0x000005C0, 2, "K6-2/3+ processor"},
    {0x00000600, 1, "Pentium Pro Rev. A processor"},
    {0x00000600, 2, "Athlon processor, 25um"},
    {0x00000600, 8, "mII (6x86MX) processor"},
    {0x00000610, 1, "Pentium Pro processor"},
    {0x00000620, 2, "Athlon processor, 18um"},
    {0x00000630, 1, "Pentium II processor, model 03"},
    {0x00000630, 2, "Duron processor"},
    {0x00000640, 2, "Athlon (Thunderbird) processor"},
    {0x00000650, 1, "Pentium II {,Xeon,Celeron} processor, model 05"},
    {0x00000650, 8, "VIA M2 processor"},
    {0x00000660, 1, "Celeron processor, model 06"},
    {0x00000660, 2, "Athlon (Palamino) processor"},
    {0x00000660, 8, "WinChip C5A processor"},
    {0x00000670, 1, "Pentium II {,Xeon} processor, model 07"},
    {0x00000670, 2, "Duron (Morgan) processor"},
    {0x00000670, 8, "WinChip C5B/C processor"},
    {0x00000680, 1, "Pentium II {,Xeon,Celeron} processor, model 08"},
    {0x00000680, 2, "Athlon (Thoroughbred) processor"},
    {0x00000680, 8, "WinChip C5N processor"},
    {0x00000690, 1, "Pentium/Celeron M processor, model 09"},
    {0x00000690, 8, "WinChip C5XL/P processor"},
    {0x000006A0, 1, "Pentium III Xeon processor, model 0Ah"},
    {0x000006A0, 2, "Athlon (Barton) processor"},
    {0x000006B0, 1, "Pentium III processor, model 0Bh"},
    {0x000006D0, 1, "Pentium/Celeron M processor, 90nm"},
    {0x000006E0, 1, "Core(tm) Solo/Duo processor, 65nm"},
    {0x000006F0, 1, "Core(tm)2 or Xeon processor, 65nm"},
    {0x00010660, 1, "Celeron processor, 65nm"},
    {0x00010670, 1, "Core(tm)2 Extreme or Xeon processor, 45nm"},
    {0x00001630, 1, "Pentium II OverDrive processor"},
    {0x00000F00, 1, "Pentium 4 style processor, 180nm"},
    {0x00000F10, 1, "Pentium 4 style processor, 160nm"},
    {0x00000F20, 1, "Pentium 4 style processor, 130nm"},
    {0x00000F30, 1, "Pentium 4 style processor, 90nm"},
    {0x00000F40, 1, "Pentium 4 style processor, 90nm"},
    {0x00000F60, 1, "Pentium 4 style processor, 65nm"},
    {0, 0, NULL},
};

const char *
get_fmsid_string(u_int32_t fmsid)
{
    static char idstr[64];
    const char *mfgstr, *typestr;
    struct fmsid_map *p;
    u_int32_t mfgid;

    /* Simple map for manufacturer */
    mfgstr = brand_leaf(0, &mfgid);

    /* Scan table for rest */
    for(p = fmsid_map; p->val; p++) {
        if(p->key == (fmsid & 0x0FFF3FF0) && (p->mfg & mfgid))
            break;
    }

    typestr = p->val;
    if(!typestr)
        typestr = "Unknown CPU Type";

    /* Generate a reasonable approximation of a cpu string */
    snprintf(idstr, 64, "%s %s", mfgstr, typestr);

    return idstr;
}

struct brandid_map {
    u_int32_t key;
    const char *val;	/* These need to be less than 48 bytes */
} brandid_map[] = {
    {0x01, "Intel(R) Celeron(R) processor"},
    {0x02, "Intel(R) Pentium(R) III processor"},
    {0x03, "Intel(R) Pentium(R) Xeon(TM) processor"},
    {0x04, "Intel(R) Pentium(R) III processor"},
    {0x06, "Mobile Intel(R) Pentium(R) III Processor-M"},
    {0x07, "Mobile Intel(R) Celeron(R) processor"},
    {0x08, "Intel(R) Pentium(R) 4 processor"},
    {0x09, "Intel(R) Pentium(R) 4 processor"},
    {0x0A, "Intel(R) Celeron(R) Processor"},
    {0x0B, "Intel(R) Xeon(TM) processor"},
    {0x0C, "Intel(R) Xeon(TM) processor MP"},
    {0x0E, "Mobile Intel(R) Pentium(R) 4 processor-M"},
    {0x0F, "Mobile Intel(R) Celeron(R) processor"},
    {0x11, "Mobile Genuine Intel(R) processor"},
    {0x12, "Intel(R) Celeron(R) M processor"},
    {0x13, "Mobile Intel(R) Celeron(R) processor"},
    {0x14, "Intel(R) Celeron(R) Processor"},
    {0x15, "Mobile Genuine Intel(R) processor"},
    {0x16, "Intel(R) Pentium(R) M processor"},
    {0x17, "Mobile Intel(R) Celeron(R) processor"},
    {0x00, NULL},
};

const char *
get_brandid_string(u_int32_t brandid, u_int32_t fms)
{
    struct brandid_map *p;

    /* Handle exceptions */
    if(brandid == 0x03 && fms == 0x6B1)
        return "Intel(R) Celeron(R) processor";
    if(brandid == 0x08 && fms >= 0xF13)
        return "Intel(R) Genuine processor";
    if(brandid == 0x0B && fms < 0xF13)
        return "Intel(R) Xeon(TM) processor MP";
    if(brandid == 0x0E && fms < 0xF13)
        return "Intel(R) Xeon(TM) processor";

    /* Scan table for rest */
    for(p = brandid_map; p->val; p++)
        if(p->key == brandid)
            break;

    return p->val;
}

/*
 cpu0: Intel(R) Core(TM)2 Duo CPU T9500 @ 2.60GHz, 2593.90 MHz
 cpu0: FPU,VME,DE,PSE,TSC,MSR,PAE,MCE,CX8,APIC,SEP,MTRR,PGE,MCA,CMOV,PAT,PSE36,CFLUSH,DS,ACPI,MMX,FXSR,SSE,SSE2,SS,HTT,TM,SBF,SSE3,MWAIT,DS-CPL,VMX,EST,TM2,CX16,xTPR,NXE,LONG
 cpu0: 6MB 64b/line 16-way L2 cache
 cpu0: apic clock running at 199MHz
 */
void
identify_shortcpu(void)
{
    struct cpuid_regs r;
    //	char brand_str[49] = "(Unknown CPU Brand String)";
    char type_buf[13];
    u_int32_t leafmax;

    /* Try CPU Brand string first */
    if ((leafmax = check_leaf(0x80000000)) && leafmax >= 0x80000005) {
        r = cpuid(0x80000002, 0);
        //		memcpy(&brand_buf[0], &r, 16);
        r = cpuid(0x80000003, 0);
        //		memcpy(&brand_buf[16], &r, 16);
        r = cpuid(0x80000004, 0);
        //		memcpy(&brand_buf[32], &r, 16);
        //		brand_buf[48] = '\0';
        //		munge_brandstr(brand_buf, brand_str);
    } else {
        /* Try BrandID if we have it */
        leafmax = check_leaf(0x00000000);
    }

    /* Base CPUID support */
    r = cpuid(0, 0);
    memcpy(&type_buf[0], &r.ebx, 4);
    memcpy(&type_buf[4], &r.edx, 4);
    memcpy(&type_buf[8], &r.ecx, 4);
    type_buf[12] = 0;

}

void
identify_cpu(void)
{
    struct cpuid_regs nregs;
    u_int32_t num_base, num_ext, FMS, brandid;
    char type_buf[13], brand_buf[49] = "";
    char brand_str[49] = "(Unknown CPU Brand String)";

    /* Ask for base CPUID support */
    nregs = cpuid(0, 0);
    num_base = nregs.eax;
    memcpy(&type_buf[0], &nregs.ebx, 4);
    memcpy(&type_buf[4], &nregs.edx, 4);
    memcpy(&type_buf[8], &nregs.ecx, 4);
    type_buf[12] = '\0';

    printf("Base: '%s', max cpuid level = %d\n", type_buf, num_base);

    /* See if we have a resonable CPU brand string */
    if(!strcmp(type_buf, "GenuineIntel")){
    }else if(!strcmp(type_buf, "AuthenticAMD")){
    }else if(!strcmp(type_buf, "GenuineTMx86")){
    }else if(!strcmp(type_buf, "CyrixInstead")){
    }else if(!strcmp(type_buf, "UMC UMC UMC ")){
    }else if(!strcmp(type_buf, "NexGenDriven")){
    }else if(!strcmp(type_buf, "CentaurHauls")){
    }else if(!strcmp(type_buf, "RiseRiseRise")){
    }else if(!strcmp(type_buf, "SiS SiS SiS ")){
    }else if(!strcmp(type_buf, "Geode by NSC")){
    }else{
        printf("Base: Unknown CPU type/vendor string!\n");
    }

    /* Decode other base stuff */
    if(num_base >= 1){
        u_int32_t ext_family, ext_model, type, family, model, step;
        u_int32_t f, m, s;
        u_int32_t htt, pcores, lcores;
        int i;

        char *proc_type[] = {
            "Original OEM processor",
            "OverDrive processor",
            "Dual processor",
            "Intel reserved (Do Not Use)",
        };

        /* Processor Signature */
        nregs = cpuid(1, 0);
        ext_family = bitx(nregs.eax, 27, 20);
        ext_model  = bitx(nregs.eax, 19, 16);
        type       = bitx(nregs.eax, 15, 12);
        family     = bitx(nregs.eax, 11, 8);
        model      = bitx(nregs.eax,  7, 4);
        step       = bitx(nregs.eax,  3, 0);

        printf("Base: Ext Family/Model = %d/%d\n", ext_family, ext_model);
        printf("Base: Family/Model/Step = %d/%d/%d\n", family, model, step);
        printf("Base: Type = '%s'\n", proc_type[type]);

        f = ext_family + family;
        m = (ext_model << 4) + model;
        s = step;
        FMS =  (f & 0xFFF) << 16;
        FMS |= (m & 0xFF) << 8;
        FMS |= (s & 0xFF);
        printf("Base: FMS = 0x%.7lx\n", (unsigned long)FMS);

        /* APIC ID, etc */
        printf("Base: APIC ID = %d\n", bitx(nregs.ebx, 31, 24));
        printf("Base: Count = %d\n", bitx(nregs.ebx, 23, 16));
        printf("Base: Chunks = %d\n", bitx(nregs.ebx, 15, 8));

        /* Possible #2 cpu string id */
        brandid = bitx(nregs.ebx, 7, 0);
        printf("Base: BrandID = %d\n", brandid);

        /* See if we do HTT or have multiple cores */
        htt = bitx(nregs.edx, 28, 28);
        lcores = bitx(nregs.ebx, 23, 16);
        if(num_base >= 4)
            pcores = bitx(cpuid(4, 0).eax, 31, 26);
        else
            pcores = 0;
        pcores += 1;

        printf("Base: HTT = %d, LCores = %d, PCores = %d\n",
               htt, lcores, pcores);

        /* Decode and print feature bits */
        for(i = 0; cd_feature[i].sname; i++){
            u_int32_t regs[4];

            regs[0] = regs[1] = 0;
            regs[2] = nregs.ecx;
            regs[3] = nregs.edx;

            if(bitx(regs[cd_feature[i].reg], cd_feature[i].bit, cd_feature[i].bit))
                printf("Base-Feature: %s, %s\n", cd_feature[i].sname, cd_feature[i].lname);
        }
    }

    /* Decode "Old" Cache information */
    /* Decode Cache information.  This is next to stupid.  Intel gives us
     * roughly 4 ways to figure out cache information.  One of them is
     * with FMS, another with cache descriptors, another with leaf #4, and
     * another God would only know.
     *
     * We check if it looks like we have valid leaf #4 information, and if
     * we do, we use it.  Otherwise we check and see if we have leaf #2,
     * and try to parse that.  That fails, punt on getting cache info.
     *
     * For a laugh, read:
     *
     * http://softwarecommunity.intel.com/isn/Community/en-US/forums/\
     *    thread/30220677.aspx
     *
     * Idiots.
     */
    /* Old leaf, decode anyways so we get TLB/etc info not in new leaf */
    if (num_base >= 2 && (cpuid(2, 0).eax & 0xFF) >= 1) {
        u_int8_t cache_buf[sizeof(struct cpuid_regs) * 256];
        unsigned int call_num, i;

        nregs = cpuid(2, 0);
        call_num = nregs.eax & 0xFF;
        printf("Base-Cache-Desc: %d page(s) of cache descriptors\n", call_num);

        memset(cache_buf, 0, sizeof(cache_buf));
        do {
            u_int32_t reg[4];

            memset(reg, 0, sizeof(reg));

            if (!(nregs.eax & 0x80000000))
                reg[0] = bswap32(nregs.eax & 0xffffff00);
            if (!(nregs.ebx & 0x80000000))
                reg[1] = bswap32(nregs.ebx);
            if (!(nregs.ecx & 0x80000000))
                reg[2] = bswap32(nregs.ecx);
            if (!(nregs.edx & 0x80000000))
                reg[3] = bswap32(nregs.edx);

            memcpy(&cache_buf[sizeof(reg) * call_num], reg, sizeof(reg));

            nregs = cpuid(2, 0);
        } while(--call_num);

        for(i = 0; i < sizeof(cache_buf); i++){
            const cache_info_t *p;

            /* No info here */
            if(!cache_buf[i])
                continue;

            /* Marker for "no more caches further on" */
            if (cache_buf[i] == 0x40) {
                printf("Base-Cache-Desc(0x%X): Found EOC indicator!\n",
                       cache_buf[i]);
                /* XXX - should be used to disabiguate 2nd/3rd level caches */
                continue;
            }

            p = ci_find(cache_buf[i]);
            if (p) {
                printf("Base-Cache-Desc(0x%X): ", cache_buf[i]);
                ci_print(p);
            } else
                printf("Base-Cache-Desc: Unknown Descriptor: 0x%.2x\n",
                       cache_buf[i]);
        }
    }

    /* Processor Serial Number */
    if (num_base >= 3 && bitx(cpuid(1, 0).edx, 18, 18)) {
        u_int32_t sig[3];

        sig[0] = cpuid(1, 0).eax;
        sig[1] = cpuid(3, 0).edx;
        sig[2] = cpuid(3, 0).ecx;

        printf("Base-PSN: 0x%.8X-%.8X-%.8X\n", sig[0], sig[1], sig[2]);

    } else if (num_base >= 3) {
        printf("Base-PSN: disabled\n");
    }

    /* New leaf for cache information */
    if (num_base >= 4 && bitx(cpuid(4, 0).eax, 3, 0) < 4) {
        u_int32_t level, full, init, type;
        u_int32_t assoc, ppart, sline, nset;
        u_int32_t prefetch, cache_size, nthr;
        int i;

        const char *st[] = { "Unknown Cache", "Data Cache",
        "Instruction Cache", "Unified Cache", NULL,
        };

        /* Limit to 1024 caches... */
        for(i = 0; i < 1024; i++){
            nregs = cpuid(4, i);
            type = nregs.eax & 0x0F;
            if (type == 0) break;
            if (type >= 4) type = 0;

            nthr = bitx(nregs.eax, 25, 14) + 1;
            full = bitx(nregs.eax, 9, 9);
            init = bitx(nregs.eax, 8, 8);
            level = bitx(nregs.eax, 7, 5);

            assoc = bitx(nregs.ebx, 31, 22) + 1;
            ppart = bitx(nregs.ebx, 21, 12) + 1;
            sline = bitx(nregs.ebx, 11, 0) + 1;

            nset = nregs.ecx + 1;

            prefetch = bitx(nregs.edx, 9, 0);
            if(prefetch == 0) prefetch = 64;

            cache_size = assoc * ppart * sline * nset;

            printf("Base-Cache-Det: nthr=%d, full=%d, init=%d\n",
                   nthr, full, init);
            printf("Base-Cache-Det: ppart=%d, sline=%d, nset=%d, pref=%d\n",
                   ppart, sline, nset, prefetch);
            printf("Base-Cache-Det: %sB %d-way %d byte/line level-%d %s\n",
                   humanize(cache_size), assoc, sline, level, st[type]);
        }

    }

    /* Check for Vendor CPUID support */
    nregs = cpuid(0x80000000, 0);
    if ((nregs.eax & 0xffff0000) == 0x80000000)
        num_ext = nregs.eax;
    else
        num_ext = 0;
    if (num_ext >= 0x80000004) {
        struct cpuid_regs r;

        r = cpuid(0x80000002, 0);
        memcpy(&brand_buf[0], &r, 16);
        r = cpuid(0x80000003, 0);
        memcpy(&brand_buf[16], &r, 16);
        r = cpuid(0x80000004, 0);
        memcpy(&brand_buf[32], &r, 16);
        brand_buf[48] = '\0';
        munge_brandstr(brand_buf, brand_str);
    }else{
        /* Non brand string support id of cpu */
        const char *p = NULL;

        if(brandid && (p = get_brandid_string(brandid, FMS))) {
            munge_brandstr(p, brand_str);
        } else if((p = get_fmsid_string(cpuid(1, 0).eax)) != NULL) {
            munge_brandstr(p, brand_str);
            /*
             * XXX - Do brand string based on FMS and cache info
             * Right now we simply have a generic string based on
             * the FMS information.  Maybe later I write extra
             * to disambiguate based on cache information.  Maybe.
             */
        } else {
            /* XXX - Unknown CPU */
            p = "Really bad JUJU happened here!";
        }
    }

    printf("Base: CPU = '%s'\n", brand_str);
    printf("Ext: max vendor cpuid level = 0x%x\n", num_ext);
    printf("Ext: brand string = '%s'\n", brand_buf);

    /* Decode AMD extended CPUID Cache info */
    if(!strcmp(type_buf, "AuthenticAMD") && num_ext >= 5){
        u_int32_t ext = 0x80000005;
        printf("L1DTlb2/4MAssoc = %d\n", bitx(cpuid(ext, 0).eax, 31, 24));
        printf("L1ITlb2/4MSize = %d\n",  bitx(cpuid(ext, 0).eax, 23, 16));
        printf("L1ITlb2/4MAssoc = %d\n", bitx(cpuid(ext, 0).eax, 15,  8));
        printf("L1ITlb2/4MSize = %d\n",  bitx(cpuid(ext, 0).eax,  7,  0));

        printf("L1DTlb4KAssoc = %d\n", bitx(cpuid(ext, 0).ebx, 31, 24));
        printf("L1DTlb4KSize = %d\n",  bitx(cpuid(ext, 0).ebx, 23, 16));
        printf("L1ITlb4KAssoc = %d\n", bitx(cpuid(ext, 0).ebx, 15,  8));
        printf("L1ITlb4KSize = %d\n",  bitx(cpuid(ext, 0).ebx,  7,  0));

        printf("L1DcSize = %d\n",      bitx(cpuid(ext, 0).ecx, 31, 24));
        printf("L1DcAssoc = %d\n",     bitx(cpuid(ext, 0).ecx, 23, 16));
        printf("L1DcLines/Tag = %d\n", bitx(cpuid(ext, 0).ecx, 15,  8));
        printf("L1DcLineSize = %d\n",  bitx(cpuid(ext, 0).ecx,  7,  0));

        printf("L1IcSize = %d\n",      bitx(cpuid(ext, 0).edx, 31, 24));
        printf("L1IcAssoc = %d\n",     bitx(cpuid(ext, 0).edx, 23, 16));
        printf("L1IcLines/Tag = %d\n", bitx(cpuid(ext, 0).edx, 15,  8));
        printf("L1IcLineSize = %d\n",  bitx(cpuid(ext, 0).edx,  7,  0));
    }
    if(!strcmp(type_buf, "AuthenticAMD") && num_ext >= 6){
        u_int32_t ext = 0x80000006;

        printf("L2DTlb2/4MAssoc = %d\n", bitx(cpuid(ext, 0).eax, 31, 28));
        printf("L2DTlb2/4MSize = %d\n",  bitx(cpuid(ext, 0).eax, 27, 16));
        printf("L2ITlb2/4MAssoc = %d\n", bitx(cpuid(ext, 0).eax, 15, 12));
        printf("L2ITlb2/4MSize = %d\n",  bitx(cpuid(ext, 0).eax, 11,  0));

        printf("L2DTlb4KAssoc = %d\n", bitx(cpuid(ext, 0).ebx, 31, 28));
        printf("L2DTlb4KSize = %d\n",  bitx(cpuid(ext, 0).ebx, 27, 16));
        printf("L2ITlb4KAssoc = %d\n", bitx(cpuid(ext, 0).ebx, 15, 12));
        printf("L2ITlb4KSize = %d\n",  bitx(cpuid(ext, 0).ebx, 11,  0));

        printf("L2Size = %d\n",      bitx(cpuid(ext, 0).ecx, 31, 16));
        printf("L2Assoc = %d\n",     bitx(cpuid(ext, 0).ecx, 15, 12));
        printf("L2Lines/Tag = %d\n", bitx(cpuid(ext, 0).ecx, 11,  8));
        printf("L2LineSize = %d\n",  bitx(cpuid(ext, 0).ecx,  7,  0));

        printf("L3Size = %d\n",      bitx(cpuid(ext, 0).edx, 31, 16));
        printf("L3Assoc = %d\n",     bitx(cpuid(ext, 0).edx, 15, 12));
        printf("L3Lines/Tag = %d\n", bitx(cpuid(ext, 0).edx, 11,  8));
        printf("L3LineSize = %d\n",  bitx(cpuid(ext, 0).edx,  7,  0));
    }
}

void
identify_hypervisor(void)
{
    struct cpuid_regs regs;
    char hyper_buf[13];
    u_int32_t num_hyper;

    /* Check if we are running in a hypervisor */
    if (!(cpuid(1, 0).ecx & 0x80000000))
        return;

    /* Get type of hypervisor we are */
    regs = cpuid(0x40000000, 0);
    num_hyper = regs.eax;
    memcpy(&hyper_buf[0], &regs.ebx, 4);
    memcpy(&hyper_buf[4], &regs.ecx, 4);
    memcpy(&hyper_buf[8], &regs.edx, 4);
    hyper_buf[12] = '\0';

    printf("Hypervisor: '%s', max hyper level = 0x%x\n", hyper_buf, num_hyper);

    if(!strcmp(hyper_buf, "VMwareVMware")){
        /*
         * XXX - leaf 0x10 is vmware specific, but they are putting the
         * spec for it out there for the public to see and follow.  So
         * we parse it later on in case M$ and others decide to implement
         * the VMware leafs.
         */
    }else if(!strcmp(hyper_buf, "XenVMMXenVMM")){
        /*
         * XXX - Xen is out on its own leaf (sic), thankfully only 2
         * leafs are defined.  Since they offer no extra signature, and
         * nobody else offers these, we parse these only for Xen VM.
         */
        printf("Hyper: Xen V%d.%d\n", bitx(cpuid(0x40000001, 0).eax, 31, 16),
               bitx(cpuid(0x40000001, 0).eax, 15, 0));
        printf("Hyper: %d transfer pages, MSR Base = 0x%X\n",
               cpuid(0x40000002, 0).eax, cpuid(0x40000002, 0).ebx);
        printf("Hyper: Features 0x%x, 0x%x\n",
               cpuid(0x40000002, 0).ecx, cpuid(0x40000002, 0).edx);

    }else if(!strcmp(hyper_buf, "KVMKVMKVM")){
        /*
         * XXX - KVM (of course) is also out on its own leaf.  Thankfully
         * also only 1 leaf (at this time).  No extra signature, so we
         * handle them here as well.
         */
        printf("Hyper: KVM CLOCKSOURCE = %d\n",
               bitx(cpuid(0x40000001, 0).eax, 0, 0));
        printf("Hyper: KVM NOP_IO_DELAY = %d\n",
               bitx(cpuid(0x40000001, 0).eax, 1, 1));
        printf("Hyper: KVM MMU_OP = %d\n",
               bitx(cpuid(0x40000001, 0).eax, 2, 2));

    }else if(!strcmp(hyper_buf, "Microsoft Hv")){
        /*
         * XXX - M$ defines leafs 0x2 - 0x7.  They do make most (if not all)
         * of the spec available online.  Since there is an extra signature
         * required for it, we do not parse the M$ stuff here, but simply
         * check for the signature, in case other hypervisors decide to
         * implement portions of the M$ leafs.
         */
    }else{
        printf("Base: Unknown Hypervisor type string!\n");
    }

    /* M$ Hypervisor from 2-7 (allow others to implement) */
    if (num_hyper >= 0x40000002 && cpuid(0x40000001, 0).eax == 0x31237648) {
        /*
         * XXX - We have to set the OS MSR ID register here.  M$ gives
         * msr HV_X64_MSR_GUEST_OS_ID = 0x40000000  as the location.
         *
         * The contents are basically:
         *
         * bits 63:48 - Vendor ID
         * bits 47:40 - OS ID
         * bits 39:32 - Major Version
         * bits 31:24 - Minor Version
         * bits 23:16 - Service Version
         * bits 15:0  - Build Number
         *
         * Vendor ID: 0 = Reserved, 1 = M$
         * OS ID: 0 = Undef, 1 = MS-DOS, 2 = M$ Win 3.x, 3 = Win 9x,
         *        4 = Win NT/etc, 5 = WinCE
         * Major/Minor Version: version of OS
         * Service/Build: service pack number, build number
         *
         * Since we can not set the OS MSR ID register, we are hoping that
         * that some OS hosting us or boot before us has already set it and
         * not un-set it.  Stupid, but it is all we have...
         * Hmm... maybe userland can set the MSR?  Try it later...
         */
        if (num_hyper >= 0x40000002) {
            regs = cpuid(0x40000002, 0);

            printf("Hyper: build %d, Version %d.%d\n", regs.eax,
                   bitx(regs.ebx, 31, 16), bitx(regs.ebx, 15, 0));
            printf("Hyper: Service pack %d, branch %d, number %d\n",
                   regs.ecx, bitx(regs.edx, 31, 24), bitx(regs.edx, 23, 0));
        }
        if (num_hyper >= 0x40000003) {
            regs = cpuid(0x40000003, 0);

            if (regs.eax & 0x01) printf("Hyper: VP runtime\n");
            if (regs.eax & 0x02) printf("Hyper: Partition reference counter\n");
            if (regs.eax & 0x04) printf("Hyper: Basic SyncIC MSRs\n");
            if (regs.eax & 0x08) printf("Hyper: Synthetic timer MSRs\n");
            if (regs.eax & 0x10) printf("Hyper: APIC access MSRs\n");
            if (regs.eax & 0x20) printf("Hyper: Hypercall MSRs\n");
            if (regs.eax & 0x40) printf("Hyper: Access virt. cpu index MSR\n");
            if (regs.eax & 0x80) printf("Hyper: Virtual system reset MSR\n");

            if (regs.eax & 0x001) printf("Hyper: CreatePartitions\n");
            if (regs.eax & 0x002) printf("Hyper: AccessPartitionId\n");
            if (regs.eax & 0x004) printf("Hyper: AccessMemoryPool\n");
            if (regs.eax & 0x008) printf("Hyper: AdjustMessageBuffers\n");
            if (regs.eax & 0x010) printf("Hyper: PostMessages\n");
            if (regs.eax & 0x020) printf("Hyper: SignalEvents\n");
            if (regs.eax & 0x040) printf("Hyper: CreatePort\n");
            if (regs.eax & 0x080) printf("Hyper: ConnectPort\n");
            if (regs.eax & 0x100) printf("Hyper: AccessStats\n");
            if (regs.eax & 0x200) printf("Hyper: IteratePhysicalHardware\n");
            if (regs.eax & 0x400) printf("Hyper: DeprecatedExposeHT\n");
            if (regs.eax & 0x800) printf("Hyper: Debugging\n");
            if (regs.eax & 0x1000) printf("Hyper: CpuPowerManagement\n");

            printf("Hyper: Max cpu power state C%d\n", bitx(regs.ecx, 3, 0));

            if (regs.edx & 0x1) printf("Hyper: MWAIT/MONITOR available\n");
            if (regs.edx & 0x2) printf("Hyper: Guest debugging\n");
            if (regs.edx & 0x4) printf("Hyper: Performance monitor support\n");
        }
        if (num_hyper >= 0x40000004) {
            regs = cpuid(0x40000004, 0);

            if (regs.eax & 0x01) printf("Hyper: slow 'mov %%cr3'\n");
            if (regs.eax & 0x02) printf("Hyper: slow 'invlpg'\n");
            if (regs.eax & 0x04) printf("Hyper: slow 'ipi tlb flush'\n");
            if (regs.eax & 0x08) printf("Hyper: slow APIC memory regs\n");
            if (regs.eax & 0x10) printf("Hyper: use MSR for system RESET\n");

            printf("Hyper: recommended spinlock failure count = %d\n",
                   regs.ebx);
        }
        if (num_hyper >= 0x40000005) {
            regs = cpuid(0x40000005, 0);

            printf("Hyper: %d max virtual CPUs\n", regs.eax);
            printf("Hyper: %d max physical CPUs\n", regs.ebx);
        }
    }

    /* VMware defined leaf, but they want it generic */
    if (num_hyper >= 0x40000010) {
        printf("Hyper: TSC %d kHz\n", cpuid(0x40000010, 0).eax);
        printf("Hyper: APIC %d kHz\n", cpuid(0x40000010, 0).ebx);
    }
}

void
dump_fullcpu(u_int32_t base)
{
    struct cpuid_regs regs;
    u_int32_t num, max;

    /* Dump everything (heuristic) */
    base &= 0xffff0000;
    regs = cpuid(base, 0);
    if((regs.eax & 0xffff0000) != base)
        return;
    max = regs.eax;
    for(num = base; num <= max; num++){
        regs = cpuid(num, 0);
        printf("0x%.8x: ", num);
        dump_regs(regs);
    }
}

void
get_cpu_class(int *class, int *cpuid_ok)
{
    *class = 6;
    *cpuid_ok = 0;

    /* First check AC (alignment check) flag */
    if (!flag_AC()) {
        *class = 3;
    } else if (!flag_ID()) {
        if (flag_VIF() && flag_VIP()) {
            *class = 5;
        } else {
            *class = 4;
        }
    } else
        *cpuid_ok = 1;
}


#if 0
int
main()
{
    int i;
    int cpu_class, cpuid_ok;



    /* First check if we are ancient or reasonably new */
    get_cpu_class(&cpu_class, &cpuid_ok);
    if(!cpuid_ok){
        printf("Ancient %d86 class CPU with no CPUID support.\n", cpu_class);
        return 0;
    } else {
        int family = bitx(cpuid(1, 0).ebx, 11, 8);

        if (family && (cpu_class > family))
            cpu_class = family;
    }

    /* Print out a short ID line */
    identify_shortcpu();

    {
        {
            identify_cpu();
            identify_hypervisor();
        }
        {
            dump_fullcpu(0x00000000);	/* Normal 'base' set */
            dump_fullcpu(0x80000000);	/* Normal 'extended' set */
            dump_fullcpu(0x80860000);	/* Someone had to be different */
            dump_fullcpu(0xC0000000);	/* Ditto, of course */
            dump_fullcpu(0x40000000);	/* Xen/VMWare/etc dump */
        }
    }


    return 0;
}

#endif


