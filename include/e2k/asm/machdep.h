#ifndef _E2K_MACHDEP_H_
#define _E2K_MACHDEP_H_

//#include <linux/init.h>
//#include <linux/types.h>

#include <sys/types.h>

#include <asm/sections.h>

#ifdef __KERNEL__

/*
 * IMPORTANT: instruction sets are numbered in increasing order,
 * each next iset being backwards compatible with all the
 * previous ones.
 */
typedef enum e2k_iset_ver {
	E2K_ISET_V1,
	E2K_ISET_V2,
	E2K_ISET_V3,
	E2K_ISET_V4,
	E2K_ISET_V5,
} e2k_iset_ver_t;

#define E2K_ISET_V1_MASK	(1 << E2K_ISET_V1)
#define E2K_ISET_V2_MASK	(1 << E2K_ISET_V2)
#define E2K_ISET_V3_MASK	(1 << E2K_ISET_V3)
#define E2K_ISET_V4_MASK	(1 << E2K_ISET_V4)
#define E2K_ISET_V5_MASK	(1 << E2K_ISET_V5)

#define E2K_ISET_SINCE_V1_MASK	(-1)
#define E2K_ISET_SINCE_V2_MASK	(E2K_ISET_SINCE_V1_MASK & ~E2K_ISET_V1_MASK)
#define E2K_ISET_SINCE_V3_MASK	(E2K_ISET_SINCE_V2_MASK & ~E2K_ISET_V2_MASK)

enum {
	/* E3M, Hybrid */
	ELBRUS_ISET = E2K_ISET_V1,
	/* E3S, Cubic, Turmalin */
	ELBRUS_S_ISET = E2K_ISET_V2,
	/* E2S (E4C) */
	ELBRUS_2S_ISET = E2K_ISET_V3,
	/* E8C (E4C+) */
	ELBRUS_8C_ISET = E2K_ISET_V4,
	/* E1C+ */
	ELBRUS_1CP_ISET = E2K_ISET_V4,
	/* E8C2 */
	ELBRUS_8C2_ISET = E2K_ISET_V5,
};

enum {
	/* #41356 - 'strd (fmt==5)' loses tags in its secondary cluster.
	 * Workaround - flush cache line before or after the store. */
	CPU_HWBUG_QUADRO_STRD,
	/* #47176 - Large pages do not work.
	 * Workaround - do not use them. */
	CPU_HWBUG_LARGE_PAGES,
	/* #54222 - Memory controller error while softreset.
	 * Workaround - complicated reset procedure. */
	CPU_HWBUG_MC_SOFTRESET,
	/* #56947 - lapic timer can lose interrupts.
	 * Workaround - do not use oneshot mode. */
	CPU_HWBUG_LAPIC_TIMER,
	/* #71610 - Atomic operations can be non-atomic
	 * Workaround - flush data cache line. */
	CPU_HWBUG_ATOMIC,
	/* #67489 - Directory cache should be disabled while softreset.
	 * Workaround - disable it. */
	CPU_HWBUG_DIRCACHE_DISABLE,
	/* #58397, #76626 - CLW does not work.
	 * Workaround - do not use it. */
	CPU_HWBUG_CLW,
	/* #76626 - "Page accessed" bit in PTE does not work.
	 * Workaround - always set it. */
	CPU_HWBUG_PAGE_A,
	/* Older processors have asyncronous SPILL/FILL */
	CPU_FEAT_ASYNC_FLUSH,
	/* On some processor's revisions writecombine memory
	 * in prefetchable PCI area is not allowed. */
	CPU_FEAT_WC_PCI_PREFETCH,
	NR_CPU_FEATURES
};


struct cpuinfo_e2k;
struct pt_regs;
struct seq_file;

typedef struct machdep {
	int		id;		/* machine Id */
	int		virt_id;	/* virtual machine Id */

	int		rev;		/* cpu revision */

	int		(*get_irq_vector)(void);

	unsigned long cpu_features[(NR_CPU_FEATURES + 63) / 64];

	u8		iset_ver;	/* Instruction set version */
	struct iset	*iset;		/* Instruction set */

	void		(*setup_arch)(void);
	void		(*setup_cpu_info)(struct cpuinfo_e2k *c);
	int		(*show_cpuinfo)(struct seq_file *m, void *v);
	void		(*init_IRQ)(int recovery_flag);

	void		(*restart)(char *cmd);
	void		(*power_off)(void);
	void		(*halt)(void);
	void		(*arch_reset)(void);
	void		(*arch_halt)(void);

	e2k_addr_t	x86_io_area_base;
} machdep_t;


static inline unsigned long test_feature(struct machdep *machine, int feature)
{
	unsigned long *addr = machine->cpu_features;

	return 1UL & (addr[feature / 64] >> (feature & 63));
}

#define cpu_has(feature)	test_feature(&machine, feature)
#define boot_cpu_has(feature)	test_feature(&boot_machine, feature)

extern void cpu_set_feature(struct machdep *machine, int feature);
extern void cpu_clear_feature(struct machdep *machine, int feature);


extern machdep_t __nodedata machine;
#ifdef	CONFIG_NUMA
#define	the_node_machine(nid)	\
		((machdep_t *)__va(node_kernel_va_to_pa(nid, &machine)))
#define	node_machine		the_node_machine(numa_node_id())
#else	/* ! CONFIG_NUMA */
#define	the_node_machine(nid)	(&machine)
#define	node_machine		the_node_machine(0)
#endif	/* CONFIG_NUMA */

#define boot_setup_cpu_features(machine) setup_cpu_features(machine)
__init void setup_cpu_features(struct machdep *machine);

#endif /* __KERNEL__ */

#endif /* _E2K_MACHDEP_H_ */
