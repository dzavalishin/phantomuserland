#ifndef _E2K_IRQFLAGS_H_
#define _E2K_IRQFLAGS_H_

#ifndef __ASSEMBLY__

//#ifndef _LINUX_TRACE_IRQFLAGS_H
//# error "Do not include <asm/irqflags.h> directly; use <linux/irqflags.h> instead."
//#endif

#include <asm/system.h>

/*
 * There are two registers to control interrupts (enable/disable)
 *
 * The main register is privileged register PSR,
 *
 * the seconde is nonprivileged UPSR.
 *
 * PSR bits should enable interrupts and enable user interrupts to use UPSR
 * as control interrupts register.
 *
 * Principal difference between two registers is scope. UPSR is global
 * register: its scope is all execution, if some function enables/disables
 * interrupts in UPSR and returns to caller then caller will have enabled/
 * disabled interrupts as well. PSR is local register: its scope is current
 * function, and all invoked functions inherit its PSR state, but if invoked
 * function changes PSR and returns, then current function (caller) will see
 * own unchanged PSR state.
 *
 * (PSR is saved by call operation and is restored by return operation from
 * chine registers).
 *
 * So in PSR case, in particular, if interrupts are enabled/disabled
 * by some function call, then it is an error - interrupts enable/disable
 * state will be unchanged. But it is not error in UPSR case.
 *
 * Interrupts control using PSR requires structured kernel organization and
 * it can be permited only inheritance of interrupts enable/disable state
 * (from caller to invoked function) and it cannot be permited return of
 * interrupts enable/disable state (to caller)
 *
 * There is doubt that we should use interrupts control under UPSR
 *
 *
 * PSR and UPSR bits are used to enable and disable interrupts.
 *
 * PSR bits are used while:
 *	- A user process executes;
 *	- Trap or interrupt occures on user or kernel process, hardware
 * disables interrupts mask in PSR and PSR becomes main register to control
 * interrupts. Trap handler switches control from PSR register to UPSR
 * in the appropriate point and all following trap handling is done under
 * UPSR control;
 *	- Trap handler returns control from UPSR to PSR in the appropriate
 * point of trap handling end. Return from trap handler (DONE) restores
 * PSR from CR register and recovers interrupts control type in the trap point;
 *	- System call is same as trap (see above);
 *	- System call end is same as trap handler end (see above);
 *	- Switch from kernel process to user (exec() and signal handler)
 * is same as trap handler end. Before return to user function kernel sets
 * control under PSR and (only for signal handler) after return from user
 * recovers control under UPSR.
 *
 * Kernel cannot use standard macros, functions to enable / disable
 * interrupts same as local_irq_xxx() spin_lock_irq_xxx() ... while
 * interrupts are controled by PSR.
 *
 * UPSR bits are used by kernel while:
 *	Kernel jumpstart (system call #12) set UPSR register in the
 * initial state (where interrupts are disabled) and switches
 * control from PSR register to UPSR; From this point kernel runs
 * (except cases listed above for PSR) under UPSR interrupt bits
 */
#define	SWITCH_IRQ_TO_UPSR(disable_sge)				\
({								\
	unsigned long cur_psr = E2K_GET_DSREG_NV(psr);		\
	cur_psr |= (PSR_IE | PSR_NMIE | PSR_UIE | PSR_UNMIE);	\
	if (disable_sge)					\
		cur_psr &= ~PSR_SGE;				\
	E2K_SET_PSR_IRQ_BARRIER(cur_psr);			\
})
#define	RETURN_IRQ_TO_PSR()					\
({								\
	unsigned long cur_psr = E2K_GET_DSREG_NV(psr);		\
	cur_psr &= ~(PSR_IE | PSR_NMIE | PSR_UIE | PSR_UNMIE);	\
	E2K_SET_PSR_IRQ_BARRIER(cur_psr);			\
})

#define	UPSR_STI()						\
({								\
        condition_collect_disable_interrupt_ticks(              \
			E2K_GET_DSREG_NV(upsr) & ~UPSR_IE);     \
	E2K_SET_UPSR_IRQ_BARRIER_NO_NOPS(AW(E2K_KERNEL_UPSR_ENABLED)); \
})

#define	UPSR_CLI()						\
({								\
	E2K_SET_UPSR_IRQ_BARRIER(AW(E2K_KERNEL_UPSR_DISABLED));	\
        condition_mark_disable_interrupt_ticks(1);              \
})

#define	UPSR_SAVE_AND_CLI()					\
({								\
	unsigned long __flags = E2K_GET_DSREG_NV(upsr);		\
	E2K_SET_UPSR_IRQ_BARRIER(AW(E2K_KERNEL_UPSR_DISABLED));	\
        condition_mark_disable_interrupt_ticks(1);              \
	__flags;						\
})

#define	UPSR_SAVE()	E2K_GET_DSREG_NV(upsr)

/*
 * nmi_* versions work only with non-maskbale ones interrupts.
 */

#define upsr_nmi_irqs_disabled() \
		((E2K_GET_DSREG_NV(upsr) & UPSR_NMIE) == 0)

#define upsr_nmi_irqs_disabled_flags(flags) \
		((flags & UPSR_NMIE) == 0)


/*
 * all_* versions work on all interrupts including
 * both maskable and non-maskbale ones.
 */

#define	UPSR_ALL_STI()						\
({								\
	condition_collect_disable_interrupt_ticks(              \
		E2K_GET_DSREG_NV(upsr) & ~UPSR_IE & ~UPSR_NMIE); \
	E2K_SET_UPSR_IRQ_BARRIER(AW(E2K_KERNEL_UPSR_ENABLED));	\
})

#define	UPSR_ALL_CLI()						\
({								\
	E2K_SET_UPSR_IRQ_BARRIER(AW(E2K_KERNEL_UPSR_DISABLED_ALL)); \
	condition_mark_disable_interrupt_ticks(1);              \
})

#define	UPSR_ALL_SAVE_AND_CLI(flags)				\
({								\
	flags = E2K_GET_DSREG_NV(upsr);				\
	E2K_SET_UPSR_IRQ_BARRIER(AW(E2K_KERNEL_UPSR_DISABLED_ALL)); \
	condition_mark_disable_interrupt_ticks(1);              \
})

#define upsr_all_irqs_disabled() \
		((E2K_GET_DSREG_NV(upsr) & (UPSR_IE | UPSR_NMIE)) == 0)

#define upsr_all_irqs_disabled_flags(flags) \
		((flags & (UPSR_IE | UPSR_NMIE)) == 0)



#define psr_irqs_disabled()	((E2K_GET_DSREG_NV(psr) & PSR_IE) == 0)
#define upsr_irqs_disabled()	((E2K_GET_DSREG_NV(upsr) & UPSR_IE) == 0)

#define	psr_and_upsr_irqs_disabled()			\
({							\
	int ret;					\
	unsigned long psr = E2K_GET_DSREG_NV(psr);	\
	if ((psr & PSR_IE) == 0) {			\
		ret = 1;				\
	} else if (psr & PSR_UIE) {			\
		ret = upsr_irqs_disabled();		\
	} else {					\
		ret = 0;				\
	}						\
	ret;						\
})

#ifndef CONFIG_DEBUG_IRQ
#define __raw_irqs_disabled()	upsr_irqs_disabled()
#else
#define __raw_irqs_disabled()	psr_and_upsr_irqs_disabled()
#endif	/* ! CONFIG_DEBUG_IRQ */

#define __raw_irqs_disabled_flags(flags)	((flags & UPSR_IE) == 0)

#ifdef CONFIG_MCST_RT

#define SAVE_CURR_TIME_SWITCH_TO                                        \
{ 									\
        cpu_times[raw_smp_processor_id()].curr_time_switch_to =         \
                                                  E2K_GET_DSREG(clkr);  \
} 

#define CALCULATE_TIME_SWITCH_TO                                        \
{                                                                       \
        int cpu = raw_smp_processor_id();                               \
        cpu_times[cpu].curr_time_switch_to = E2K_GET_DSREG_NV(clkr) -   \
                              cpu_times[cpu].curr_time_switch_to;       \
        if (cpu_times[cpu].curr_time_switch_to <                        \
            cpu_times[cpu].min_time_switch_to){                         \
            cpu_times[cpu].min_time_switch_to =                         \
                               cpu_times[cpu].curr_time_switch_to;      \
        }                                                               \
        if (cpu_times[cpu].curr_time_switch_to >                        \
            cpu_times[cpu].max_time_switch_to){                         \
            cpu_times[cpu].max_time_switch_to =                         \
                                cpu_times[cpu].curr_time_switch_to;     \
        }                                                               \
}

#else /* !CONFIG_MCST_RT */
 #define SAVE_CURR_TIME_SWITCH_TO
 #define CALCULATE_TIME_SWITCH_TO
#endif /* CONFIG_MCST_RT */

#ifdef CONFIG_CLI_CHECK_TIME

typedef struct cli_info {
	long cli;
	long max_cli;
	long max_cli_cl;
	long max_cli_ip;

	long gcli;
	long max_gcli;
	long max_gcli_cl;
	long max_gcli_ip;
	
} cli_info_t;

typedef struct tt0_info {
	long max_tt0_prolog;
	long max_tt0_cl;
} tt0_info_t;

extern cli_info_t 	cli_info[];
extern tt0_info_t 	tt0_info[];
extern int 		cli_info_needed;
extern void 		tt0_prolog_ticks(long ticks);

#define Cli_cl	 	cli_info[raw_smp_processor_id()].cli
#define Max_cli  	cli_info[raw_smp_processor_id()].max_cli
#define Max_cli_cl  	cli_info[raw_smp_processor_id()].max_cli_cl
#define Max_cli_ip  	cli_info[raw_smp_processor_id()].max_cli_ip
#define Cli_irq	 	cli_info[raw_smp_processor_id()].irq

#define Gcli_cl 	cli_info[raw_smp_processor_id()].gcli
#define Max_gcli  	cli_info[raw_smp_processor_id()].max_gcli
#define Max_gcli_cl  	cli_info[raw_smp_processor_id()].max_gcli_cl
#define Max_gcli_ip  	cli_info[raw_smp_processor_id()].max_gcli_ip

#define Max_tt0_prolog 	tt0_info[raw_smp_processor_id()].max_tt0_prolog
#define Max_tt0_cl 	tt0_info[raw_smp_processor_id()].max_tt0_cl

#define	e2k_cli()							\
{ 									\
	bool __save_time = cli_info_needed && !__raw_irqs_disabled();	\
	UPSR_CLI();							\
	if (__save_time)						\
		Cli_cl = E2K_GET_DSREG(clkr);				\
}

#define	e2k_sti()							\
{ 									\
	if (Cli_cl && __raw_irqs_disabled() && 				\
		(Max_cli < E2K_GET_DSREG(clkr) - Cli_cl)) {		\
		Max_cli = E2K_GET_DSREG(clkr) - Cli_cl;			\
		Max_cli_cl = Cli_cl;					\
		Max_cli_ip = E2K_GET_DSREG(ip); 			\
	}								\
	UPSR_STI();							\
}

// check_cli() works under cli() but we want to check time of cli()

#define	check_cli()							\
{ 									\
	if (cli_info_needed) {						\
		Cli_cl = E2K_GET_DSREG(clkr); 				\
	}								\
}

#define	sti_return()							\
{ 									\
	if (cli_info_needed && __raw_irqs_disabled() && 			\
		(Max_cli < E2K_GET_DSREG(clkr) - Cli_cl)) {		\
		Max_cli = E2K_GET_DSREG(clkr) - Cli_cl;			\
		Max_cli_cl = Cli_cl;					\
		Max_cli_ip = E2K_GET_DSREG(ip); 			\
	}								\
}
#else /* above CONFIG_CLI_CHECK_TIME */
#define	e2k_cli()		UPSR_CLI()
#define	e2k_sti()		UPSR_STI()
#define check_cli()
#endif /* CONFIG_CLI_CHECK_TIME */

/* Normal irq operations: disable maskable interrupts only,
 * but enable both maskable and non-maskable interrupts. */

#define arch_local_irq_enable()		e2k_sti()
#define arch_local_irq_disable()	e2k_cli()

#define arch_local_irq_save()		UPSR_SAVE_AND_CLI()
#define arch_local_irq_restore(x)	UPSR_RESTORE(x)

#define arch_local_save_flags()		UPSR_SAVE()

#define arch_irqs_disabled_flags(x)	__raw_irqs_disabled_flags(x)
#define arch_irqs_disabled()		__raw_irqs_disabled()

/* nmi_irq_*() - the same as above, but checks only non-maskable interrupts. */

#define raw_nmi_irqs_disabled_flags(x)	upsr_nmi_irqs_disabled_flags(x)
#define raw_nmi_irqs_disabled()		upsr_nmi_irqs_disabled()

/* all_irq_*() - the same as above, but enables, disables and checks
 * both non-maskable and maskable interrupts. */

#define raw_all_irq_enable()		UPSR_ALL_STI()
#define raw_all_irq_disable()	        UPSR_ALL_CLI()

#define raw_all_irq_save(x)		UPSR_ALL_SAVE_AND_CLI(x)
#define raw_all_irq_restore(x)		UPSR_RESTORE(x)

#define raw_all_irqs_disabled_flags(x)	upsr_all_irqs_disabled_flags(x)
#define raw_all_irqs_disabled()		upsr_all_irqs_disabled()

#define all_irq_enable() \
	do { trace_hardirqs_on(); raw_all_irq_enable(); } while (0)

#define all_irq_disable() \
	do { raw_all_irq_disable(); trace_hardirqs_off(); } while (0)

#define all_irq_save(flags)				\
	do {						\
		typecheck(unsigned long, flags);	\
		raw_all_irq_save(flags);		\
		trace_hardirqs_off();			\
	} while (0)

#define all_irq_restore(flags)					\
	do {							\
		typecheck(unsigned long, flags);		\
		if (raw_all_irqs_disabled_flags(flags)) {	\
			raw_all_irq_restore(flags);		\
			trace_hardirqs_off();			\
		} else {					\
			trace_hardirqs_on();			\
			raw_all_irq_restore(flags);		\
		}						\
	} while (0)

/*
 * Used in the idle loop
 */
static inline void arch_safe_halt(void)
{
}

#endif /* __ASSEMBLY__ */
#endif /* _E2K_IRQFLAGS_H_ */
