#ifndef ARCH_ia32
#warning Intel32 code! Wrong arch?
#endif

// x86 features from cpuid eax 1, ecx register
#define X86_FPU   0x00000001 // x87 fpu
#define X86_VME   0x00000002 // virtual 8086
#define X86_DE    0x00000004 // debugging extensions
#define X86_PSE   0x00000008 // page size extensions
#define X86_TSC   0x00000010 // rdtsc instruction
#define X86_MSR   0x00000020 // rdmsr/wrmsr instruction
#define X86_PAE   0x00000040 // extended 3 level page table addressing
#define X86_MCE   0x00000080 // machine check exception
#define X86_CX8   0x00000100 // cmpxchg8b instruction
#define X86_APIC  0x00000200 // local apic on chip
#define X86_SEP   0x00000800 // SYSENTER/SYSEXIT
#define X86_MTRR  0x00001000 // MTRR
#define X86_PGE   0x00002000 // paging global bit
#define X86_MCA   0x00004000 // machine check architecture
#define X86_CMOV  0x00008000 // cmov instruction
#define X86_PAT   0x00010000 // page attribute table
#define X86_PSE36 0x00020000 // page size extensions with 4MB pages
#define X86_PSN   0x00040000 // processor serial number
#define X86_CLFSH 0x00080000 // cflush instruction
#define X86_DS    0x00200000 // debug store
#define X86_ACPI  0x00400000 // thermal monitor and clock ctrl
#define X86_MMX   0x00800000 // mmx instructions
#define X86_FXSR  0x01000000 // FXSAVE/FXRSTOR instruction
#define X86_SSE   0x02000000 // SSE
#define X86_SSE2  0x04000000 // SSE2
#define X86_SS    0x08000000 // self snoop
#define X86_HTT   0x10000000 // hyperthreading
#define X86_TM    0x20000000 // thermal monitor
#define X86_PBE   0x80000000 // pending break enable

// x86 features from cpuid eax 1, edx register
#define X86_EXT_SSE3  0x00000001 // SSE3
#define X86_EXT_MONITOR 0x00000008 // MONITOR/MWAIT
#define X86_EXT_DSCPL 0x00000010 // CPL qualified debug store
#define X86_EXT_EST   0x00000080 // speedstep
#define X86_EXT_TM2   0x00000100 // thermal monitor 2
#define X86_EXT_CNXTID 0x00000400 // L1 context ID

// x86 features from cpuid eax 0x80000001, edx register (AMD)
// only care about the ones that are unique to this register
#define X86_AMD_EXT_SYSCALL 0x00000800 // SYSCALL/SYSRET
#define X86_AMD_EXT_NX      (1<<20)    // no execute bit
#define X86_AMD_EXT_MMXEXT  (1<<22)    // mmx extensions
#define X86_AMD_EXT_FFXSR   (1<<25)    // fast FXSAVE/FXRSTOR
#define X86_AMD_EXT_LONG    (1<<29)    // long mode
#define X86_AMD_EXT_3DNOWEXT (1<<30)   // 3DNow! extensions
#define X86_AMD_EXT_3DNOW   (1<<31)   // 3DNow!

// features
enum i386_feature_type {
	FEATURE_COMMON = 0,     // cpuid eax=1, ecx register
	FEATURE_EXT,            // cpuid eax=1, edx register
	FEATURE_EXT_AMD,        // cpuid eax=0x80000001, edx register (AMD)

	FEATURE_NUM
};

enum i386_vendors {
	VENDOR_INTEL = 0,
	VENDOR_AMD,
	VENDOR_CYRIX,
	VENDOR_UMC,
	VENDOR_NEXGEN,
	VENDOR_CENTAUR,
	VENDOR_RISE,
	VENDOR_TRANSMETA,
	VENDOR_NSC,

	VENDOR_NUM,
	VENDOR_UNKNOWN,
};

struct arch_cpu_info {
	enum i386_vendors vendor;
	enum i386_feature_type feature[FEATURE_NUM];
	char model_name[49];
	const char *vendor_name;
	char feature_string[512];
	int family;
	int stepping;
	int model;
};

