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


#include <kernel/ia32/cpu.h>
#include <kernel/init.h>
#include <kernel/boot.h>

#include <string.h>
#include <phantom_libc.h>


/* cpu vendor info */
struct cpu_vendor_info {
	const char *vendor;
	const char *ident_string[2];
};

static const struct cpu_vendor_info vendor_info[9] = {
	{ "Intel", { "GenuineIntel" } },
	{ "AMD", { "AuthenticAMD" } },
	{ "Cyrix", { "CyrixInstead" } },
	{ "UMC", { "UMC UMC UMC" } },
	{ "NexGen", { "NexGenDriven" } },
	{ "Centaur", { "CentaurHauls" } },
	{ "Rise", { "RiseRiseRise" } },
	{ "Transmeta", { "GenuineTMx86", "TransmetaCPU" } },
	{ "NSC", { "Geode by NSC" } },
};






static void make_feature_string(cpu_ent *cpu, char *str)
{

	str[0] = 0;

	if(cpu->arch.feature[FEATURE_COMMON] & X86_FPU)
		strcat(str, "fpu ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_VME)
		strcat(str, "vme ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_DE)
		strcat(str, "de ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_PSE)
		strcat(str, "pse ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_TSC)
		strcat(str, "tsc ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_MSR)
		strcat(str, "msr ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_PAE)
		strcat(str, "pae ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_MCE)
		strcat(str, "mce ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_CX8)
		strcat(str, "cx8 ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_APIC)
		strcat(str, "apic ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_SEP)
		strcat(str, "sep ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_MTRR)
		strcat(str, "mtrr ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_PGE)
		strcat(str, "pge ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_MCA)
		strcat(str, "mca ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_CMOV)
		strcat(str, "cmov ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_PAT)
		strcat(str, "pat ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_PSE36)
		strcat(str, "pse36 ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_PSN)
		strcat(str, "psn ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_CLFSH)
		strcat(str, "clfsh ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_DS)
		strcat(str, "ds ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_ACPI)
		strcat(str, "acpi ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_MMX)
		strcat(str, "mmx ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_FXSR)
		strcat(str, "fxsr ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_SSE)
		strcat(str, "sse ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_SSE2)
		strcat(str, "sse2 ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_SS)
		strcat(str, "ss ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_HTT)
		strcat(str, "htt ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_TM)
		strcat(str, "tm ");
	if(cpu->arch.feature[FEATURE_COMMON] & X86_PBE)
		strcat(str, "pbe ");
	if(cpu->arch.feature[FEATURE_EXT] & X86_EXT_SSE3)
		strcat(str, "sse3 ");
	if(cpu->arch.feature[FEATURE_EXT] & X86_EXT_MONITOR)
		strcat(str, "monitor ");
	if(cpu->arch.feature[FEATURE_EXT] & X86_EXT_DSCPL)
		strcat(str, "dscpl ");
	if(cpu->arch.feature[FEATURE_EXT] & X86_EXT_EST)
		strcat(str, "est ");
	if(cpu->arch.feature[FEATURE_EXT] & X86_EXT_TM2)
		strcat(str, "tm2 ");
	if(cpu->arch.feature[FEATURE_EXT] & X86_EXT_CNXTID)
		strcat(str, "cnxtid ");
	if(cpu->arch.feature[FEATURE_EXT_AMD] & X86_AMD_EXT_SYSCALL)
		strcat(str, "syscall ");
	if(cpu->arch.feature[FEATURE_EXT_AMD] & X86_AMD_EXT_NX)
		strcat(str, "nx ");
	if(cpu->arch.feature[FEATURE_EXT_AMD] & X86_AMD_EXT_MMXEXT)
		strcat(str, "mmxext ");
	if(cpu->arch.feature[FEATURE_EXT_AMD] & X86_AMD_EXT_FFXSR)
		strcat(str, "ffxsr ");
	if(cpu->arch.feature[FEATURE_EXT_AMD] & X86_AMD_EXT_LONG)
		strcat(str, "long ");
	if(cpu->arch.feature[FEATURE_EXT_AMD] & X86_AMD_EXT_3DNOWEXT)
		strcat(str, "3dnowext ");
	if(cpu->arch.feature[FEATURE_EXT_AMD] & X86_AMD_EXT_3DNOW)
		strcat(str, "3dnow ");
}


static cpu_ent boot_cpu;

int detect_cpu(int curr_cpu)
{
	unsigned int data[4];
	char vendor_str[17];
	int i;
	cpu_ent *cpu = &boot_cpu; //get_cpu_struct(curr_cpu);

	// clear out the cpu info data
	cpu->arch.vendor = VENDOR_UNKNOWN;
	cpu->arch.vendor_name = "UNKNOWN VENDOR";
	cpu->arch.feature[FEATURE_COMMON] = 0;
	cpu->arch.feature[FEATURE_EXT] = 0;
	cpu->arch.feature[FEATURE_EXT_AMD] = 0;
	cpu->arch.model_name[0] = 0;

	// print some fun data
	i386_cpuid(0, data);

	// build the vendor string
	memset(vendor_str, 0, sizeof(vendor_str));
	*(unsigned int *)&vendor_str[0] = data[1];
	*(unsigned int *)&vendor_str[4] = data[3];
	*(unsigned int *)&vendor_str[8] = data[2];

	// get the family, model, stepping
	i386_cpuid(1, data);
	cpu->arch.family = (data[0] >> 8) & 0xf;
	cpu->arch.model = (data[0] >> 4) & 0xf;
	cpu->arch.stepping = data[0] & 0xf;

        printf("CPU %d: family %d model %d stepping %d, string '%s'\n",
		curr_cpu, cpu->arch.family, cpu->arch.model, cpu->arch.stepping, vendor_str);

	// figure out what vendor we have here

	for(i=0; i<VENDOR_NUM; i++) {
            if(!strcmp(vendor_str, vendor_info[i].ident_string[0])) {
                cpu->arch.vendor = i;
                cpu->arch.vendor_name = vendor_info[i].vendor;
                break;
            }
            if(!strcmp(vendor_str, vendor_info[i].ident_string[1])) {
                cpu->arch.vendor = i;
                cpu->arch.vendor_name = vendor_info[i].vendor;
                break;
            }
	}

	// see if we can get the model name
	i386_cpuid(0x80000000, data);
	if(data[0] >= 0x80000004) {
		// build the model string
		memset(cpu->arch.model_name, 0, sizeof(cpu->arch.model_name));
		i386_cpuid(0x80000002, data);
		memcpy(cpu->arch.model_name, data, sizeof(data));
		i386_cpuid(0x80000003, data);
		memcpy(cpu->arch.model_name+16, data, sizeof(data));
		i386_cpuid(0x80000004, data);
		memcpy(cpu->arch.model_name+32, data, sizeof(data));

		// some cpus return a right-justified string
		for(i = 0; cpu->arch.model_name[i] == ' '; i++)
			;
		if(i > 0) {
			memmove(cpu->arch.model_name, 
				&cpu->arch.model_name[i], 
				strlen(&cpu->arch.model_name[i]) + 1);
		}

		printf("CPU %d: vendor '%s' model name '%s'\n",
			curr_cpu, cpu->arch.vendor_name, cpu->arch.model_name);
	} else {
		strcpy(cpu->arch.model_name, "unknown");
	}

	// load feature bits
	i386_cpuid(1, data);
	cpu->arch.feature[FEATURE_COMMON] = data[3]; // edx
	cpu->arch.feature[FEATURE_EXT] = data[2]; // ecx
	if(cpu->arch.vendor == VENDOR_AMD) {
		i386_cpuid(0x80000001, data);
		cpu->arch.feature[FEATURE_EXT_AMD] = data[3]; // edx
	}

	make_feature_string(cpu, cpu->arch.feature_string);
	//dprintf("CPU %d: features: %s\n", curr_cpu, cpu->arch.feature_string);
	printf("CPU %d: features: %s\n", curr_cpu, cpu->arch.feature_string);

        if(boot_cpu.arch.feature[FEATURE_COMMON] & X86_APIC)
            ARCH_SET_FLAG(ARCH_IA32_APIC);

	if(cpu->arch.feature[FEATURE_COMMON] & X86_SSE2)
            ARCH_SET_FLAG(ARCH_IA32_SSE2);

	return 0;
}




/*
int boot_cpu_has_apic(void)
{
    return boot_cpu.arch.feature[FEATURE_COMMON] & X86_APIC;
}
*/
