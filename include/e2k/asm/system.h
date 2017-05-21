/*
 * asm-e2k/system.h
 */
#ifndef _E2K_SYSTEM_H_
#define _E2K_SYSTEM_H_

#ifndef __ASSEMBLY__
//#include <linux/kernel.h>
#endif	/* !(__ASSEMBLY__) */
//#include <linux/irqflags.h>
#include <asm/atomic.h>
#include <asm/mas.h>
#include <asm/cpu_regs.h>
#include <asm/e2k_api.h>
#include <asm/ptrace.h>
#include <asm/traps.h>
#include <asm/unistd.h>

#ifdef __KERNEL__

typedef unsigned long (*system_call_func)(unsigned long arg1,
						unsigned long arg2,
						unsigned long arg3,
						unsigned long arg4,
						unsigned long arg5,
						unsigned long arg6);

extern	system_call_func sys_call_table[];    /* defined in systable.c */

#endif	/* __KERNEL__ */

#define __HAVE_ARCH_CMPXCHG 1

#define set_mb(var, value)  do { var = value;  mb(); } while (0)
#define set_wmb(var, value) do { var = value; wmb(); } while (0)


#define	e2k_set_sge()	E2K_SET_SREG(psr, (E2K_GET_SREG_NV(psr) | PSR_SGE))
#define	e2k_reset_sge()	E2K_SET_SREG(psr, (E2K_GET_SREG_NV(psr) & ~PSR_SGE))

static inline int e2k_sge_is_set()
{
	e2k_psr_t psr;

	AW(psr) = E2K_GET_SREG_NV(psr);

	return AS(psr).sge;
}

#ifdef CONFIG_PROFILING
  // this is the part of <linux/smp.h> - but we can't use this include !!
#ifdef CONFIG_DEBUG_PREEMPT
  extern unsigned int debug_smp_processor_id(void);
#define smp_processor_id() debug_smp_processor_id()
#else  
#define smp_processor_id() raw_smp_processor_id()
#endif // CONFIG_DEBUG_PREEMPT

#ifdef CONFIG_SMP
# ifndef CONFIG_E2S_CPU_RF_BUG
register unsigned long long __cpu_reg __asm__ ("%g19");

#  define raw_smp_processor_id() __cpu_reg
# else
#  define raw_smp_processor_id() (current_thread_info()->cpu)
# endif
#else  
# define raw_smp_processor_id() 0
#endif // CONFIG_SMP 
 
#define boot_smp_processor_id_() \
                (((e2k_addr_t)current_thread_info() >= TASK_SIZE) ? \
                        smp_processor_id() : ((long)E2K_GET_DSREG(osr0)))
 
/*
 *  The information for future using e2k-readprofile script 
 *  To collect this info we need "profile=2" in cmdline
 *  (for optimal using kernel memory)
 *   We collect the additional info for following cases:
 *    - total time of interrupts
 *    - number of interrupts
 *    - time of each interrupt 
 *    - max time of disabled interrupt
 *    - number of sys_calls
 *    - total time of sys_calls
 *    - time of each sys_call
 */    
typedef struct {
        int  hz;                 // value HZ
        int  cpu;                // number of cpu
	int  freq;               // frequency of cpu 
	int  int_max_num;        // size of  interrupts array
                                 // (in disable_interrupt_t structure) 
	int  nr_syscalls;        // size of  syscall array
                                 // (in disable_interrupt_t structure) 
        int  dummy;
	long begin_time;         // time of beginning prof collection
	long begin_ticks;        // ticks of beginning prof collection 
	long end_time;           // time of ending prof collection
                                 // (read or write frofile file) 
	long end_ticks;          // ticks of ending prof collection
                                 // (read or write frofile file)
        long n_user;             //  number of interrupt on user 
        long n_kern;             //  number of interrupt on kern 
} additional_info_t;

typedef struct {
                                                // FIRST ELEMENT
        long max_disable_interrupt;             // max #ticks of disable_interrupt
        long sum_disable_interrupt;             // all #ticks of disable_interrupt
        long number_interrupt;                  // number of interrupts
        long number_irqs;                       // number of closed irq
        long number_system_call;                // number of system_call
        
        long max_disable_interrupt_after_dump;  // max #ticks of disable_interrupt
                                                // after last read or write profile file
        long interrupts[exc_max_num];           // interrupt table 
        long interrupts_time[exc_max_num];      // interrupt time (in ticks)
        long max_interrupts_time[exc_max_num];  // max interrupt time (in ticks)
        long syscall[NR_syscalls];              // syscall   table
        long syscall_time[NR_syscalls];         // syscall   time (in ticks)
        long max_syscall_time[NR_syscalls];     // max syscall  time
        long clk;                               // time of interrupt's begining 
        // NR_VECTORS 256
        long max_do_irq_time[256];              // max DO_IRQ's  time
        long do_irq[256];                       // number of DO_IRQ
        long do_irq_time[256];                  // time of DO_IRQ
        long clk_of_do_irq;                     // time of DO_IRQ's begining 
        long last_element;
} disable_interrupt_t ;

extern unsigned long get_cmos_time(void);
extern additional_info_t additional_info;
extern disable_interrupt_t disable_interrupt[NR_CPUS];

#define get_additional_size()                                  \
      (sizeof(additional_info) + sizeof(disable_interrupt))

#define read_additional_info(buf, ind,  n_u, n_k)               \
({    int i;                                                    \
      additional_info.end_ticks = E2K_GET_DSREG(clkr);          \
      additional_info.end_time =  get_cmos_time();              \
      additional_info.n_user =  n_u;                            \
      additional_info.n_kern =  n_k;                            \
      memcpy((void*) &buf[ind], (void*)&additional_info,        \
            sizeof(additional_info));                           \
      for (i =0; i < NR_CPUS; i++) {                     \
        disable_interrupt[i].max_disable_interrupt =            \
                 system_info[i].max_disabled_interrupt.max_time;\
        disable_interrupt[i].sum_disable_interrupt =            \
                system_info[i].max_disabled_interrupt.full_time;\
        disable_interrupt[i].number_irqs =                      \
                system_info[i].max_disabled_interrupt.number;   \
      }                                                         \
      for (i = 0; i < NR_CPUS; i++) {                    \
          disable_interrupt[i].last_element = 0x4321;           \
      }                                                         \
      memcpy((void*) &buf[ind] + sizeof(additional_info),       \
        (void*)&disable_interrupt[0].max_disable_interrupt,     \
         sizeof(disable_interrupt));                            \
      for (i =0; i < NR_CPUS; i++) {                     \
        disable_interrupt[i].max_disable_interrupt_after_dump=0;\
      }                                                         \
}) 

#define set_calibration_result(res)                             \
                     additional_info.freq = res                

#define read_ticks(n) n = E2K_GET_DSREG(clkr)
                     
#define add_info_interrupt(n, ticks)                            \
({    long t; int cpu;                                          \
      t = E2K_GET_DSREG(clkr)- ticks;                           \
      cpu = boot_smp_processor_id_();                           \
      disable_interrupt[cpu].interrupts[n]++;                   \
      disable_interrupt[cpu].interrupts_time[n]+=t;             \
      if ( t > disable_interrupt[cpu].max_interrupts_time[n]){  \
          disable_interrupt[cpu].max_interrupts_time[n] = t;    \
      }                                                         \
})      

#define add_info_syscall(n, ticks)                              \
({    long t; int cpu;                                          \
      cpu = boot_smp_processor_id_();                           \
      t = E2K_GET_DSREG(clkr)- ticks;                           \
      disable_interrupt[cpu].syscall[n]++;                      \
      disable_interrupt[cpu].syscall_time[n] +=t;               \
      if ( t > disable_interrupt[cpu].max_syscall_time[n]) {    \
          disable_interrupt[cpu].max_syscall_time[n] = t;       \
      }                                                         \
})      
#define init_additional_info()                                  \
({                                                              \
      additional_info.begin_time = get_cmos_time();             \
      additional_info.dummy = 0x1234;                           \
      additional_info.hz = HZ;                                  \
      additional_info.cpu = NR_CPUS;                     \
      additional_info.nr_syscalls = NR_syscalls;                \
      additional_info.int_max_num = exc_max_num;                \
      additional_info.begin_ticks = E2K_GET_DSREG(clkr);        \
      memset(                                                   \
            (void*)&disable_interrupt[0].max_disable_interrupt, \
               0, sizeof(disable_interrupt));                   \
})    
typedef struct {
        long max_time;
        long full_time;
        long begin_time;
        long number;
        long beg_ip;
        long beg_parent_ip;
        long end_ip;
        long end_parent_ip;
        long max_beg_ip;
        long max_beg_parent_ip;
        long max_end_ip;
        long max_begin_time; 
        long max_end_parent_ip;
} time_info_t;
/* 
 *  For adding new element you need do following things:
 *  -  add new "time_info_t" element after last one
 *  -  add name of your elemen in system_info_name (file e2k_sysworks.c)
 *  -  create two new define similar below:
 *     #define  info_save_mmu_reg(tick)                                \
 *              store_max_time_in_system_info(tick,max_mmu_reg)
 *     #define  info_save_mmu_reg(tick) - null for not CONFIG_PROFILING
 *  -  used your new define for merging what you want 
 */ 
typedef struct {
        time_info_t max_disabled_interrupt;           // max time of disabled inerrupts
        time_info_t max_stack_reg;                    // max time of saving of stack_registers
        time_info_t max_tir_reg;                      // max time for storing TIR
        time_info_t max_saving_reg;                   // max time for storing all registers
        time_info_t max_mmu_reg;                      // max time for storing mmu registers
        time_info_t max_restore_stack_reg;            // max time for restoring of stack_registers
        time_info_t max_restoring_reg;                // max time for restoring all registers
        time_info_t max_restore_mmu_reg;              // max time for restoring mmu registers
        time_info_t max_cpu_idle;                     // max time for cpu_idle
        time_info_t max_spin_lock;                    // max time for spin_lock
        time_info_t max_mutex_lock;                   // max time for mutex
        time_info_t max_hard_irq;                     // max time for hard_irq
        time_info_t max_switch;                       // max time for switch
        time_info_t max_soft_irq;                     // max time for soft_irq
        time_info_t max_preempt_count;                // max time for preempt_count
} system_info_t ;

extern char* system_info_name[];
extern system_info_t system_info[NR_CPUS];
extern int enable_collect_interrupt_ticks;
#define collect_disable_interrupt_ticks()                       \
({  int  cpu;                                                   \
    cpu = boot_smp_processor_id_();                             \
    if (system_info[cpu].max_disabled_interrupt.begin_time >0){ \
       store_max_time_in_system_info(                           \
         system_info[cpu].max_disabled_interrupt.begin_time,    \
         max_disabled_interrupt);                               \
       system_info[cpu].max_disabled_interrupt.begin_time = 0;  \
    }                                                           \
})

#define mark_disable_interrupt_ticks()                          \
      store_begin_ip_in_system_info(max_disabled_interrupt)

#define store_do_irq_ticks()                                    \
({  int  cpu = boot_smp_processor_id_();                        \
   disable_interrupt[cpu].clk_of_do_irq= E2K_GET_DSREG(clkr);   \
})

#define   define_time_of_do_irq(N)                              \
({  long t; int  cpu;                                           \
    cpu = boot_smp_processor_id_();                             \
    t= E2K_GET_DSREG(clkr)-                                     \
                       disable_interrupt[cpu].clk_of_do_irq;    \
    disable_interrupt[cpu].do_irq_time[N] += t;                 \
    disable_interrupt[cpu].do_irq[N] ++;                        \
    if (disable_interrupt[cpu].max_do_irq_time[N] < t) {        \
            disable_interrupt[cpu].max_do_irq_time[N] = t;      \
    }                                                           \
})
#define  info_save_stack_reg(tick)                              \
         store_max_time_in_system_info(tick,max_stack_reg)
#define  info_restore_stack_reg(tick)                           \
         store_max_time_in_system_info(tick,max_restore_stack_reg)


#define  info_save_mmu_reg(tick)                                \
         store_max_time_in_system_info(tick,max_mmu_reg)
    
#define  info_restore_mmu_reg(tick)                             \
         store_max_time_in_system_info(tick,max_restore_mmu_reg)
    
#define  info_save_tir_reg(tick)                                \
         store_max_time_in_system_info(tick,max_tir_reg)

#define  info_mutex(mutex)                                      \
         store_end_ip_in_system_info(mutex,max_mutex_lock);     \
         store_max_time_in_system_info(mutex->clk,              \
                                       max_mutex_lock)
    
#define  info_spin_lock(spin_lock)                              \
         store_end_ip_in_system_info(spin_lock,max_spin_lock);  \
         store_max_time_in_system_info(spin_lock->clk,          \
                                       max_spin_lock)
    
    
    
#define  info_restore_all_reg(tick)                             \
         store_max_time_in_system_info(tick,max_restoring_reg); \

#define  info_save_all_reg()                                    \
         store_max_time_in_system_info(                         \
             disable_interrupt[cpu].clk, max_saving_reg)  
#define preempt_count_time()                                    \
        store_begin_time_in_system_info(max_preempt_count)
#define calculate_preempt_count()                               \
        calculate_max_time_in_system_info(max_preempt_count)

#define switch_time()                                           \
        store_begin_time_in_system_info(max_switch)
#define calculate_switch_time()                                 \
        calculate_max_time_in_system_info(max_switch)

#define soft_irq_time()                                         \
        store_begin_time_in_system_info(max_soft_irq)
#define calculate_soft_irq_time()                               \
        calculate_max_time_in_system_info(max_soft_irq)
    
#define hard_irq_time()                                         \
        store_begin_time_in_system_info(max_hard_irq)
#define calculate_hard_irq_time()                               \
        calculate_max_time_in_system_info(max_hard_irq)
   
#define cpu_idle_time()                                         \
        store_begin_time_in_system_info(max_cpu_idle)
#define calculate_cpu_idle_time()                               \
        calculate_max_time_in_system_info(max_cpu_idle)

#define  store_begin_time_in_system_info(FIELD)                 \
({  long t; int  cpu;                                           \
    register e2k_cr0_hi_t 	cr0_hi;                         \
    if (enable_collect_interrupt_ticks) {                       \
        cpu = boot_smp_processor_id_();                         \
        t= E2K_GET_DSREG(clkr);                                 \
        system_info[cpu].FIELD.begin_time = t;                  \
        system_info[cpu].FIELD.beg_ip=E2K_GET_DSREG(ip);        \
        system_info[cpu].FIELD.beg_parent_ip=                   \
         (AS_STRUCT(cr0_hi)).ip<<3;                             \
    }                                                           \
})

#define  store_begin_ip_in_system_info(FIELD)                   \
({  int  cpu;                                                   \
    register e2k_cr0_hi_t 	cr0_hi;                         \
    cpu = boot_smp_processor_id_();                             \
    disable_interrupt[cpu].clk = E2K_GET_DSREG(clkr);           \
    AS_WORD(cr0_hi) =  E2K_GET_DSREG_NV(cr0.hi);                \
    system_info[cpu].FIELD.beg_ip = E2K_GET_DSREG(ip);          \
    system_info[cpu].FIELD.beg_parent_ip =                      \
     (AS_STRUCT(cr0_hi)).ip<<3;                                 \
})

#define  store_begin_time_ip_in_system_info(cpu, tick, FIELD)   \
({                                                              \
    register e2k_cr0_hi_t 	cr0_hi;                         \
    if (enable_collect_interrupt_ticks) {                       \
        system_info[cpu].FIELD.begin_time = tick;               \
        AS_WORD(cr0_hi) =  E2K_GET_DSREG_NV(cr0.hi);            \
        system_info[cpu].FIELD.beg_ip = E2K_GET_DSREG(ip);      \
         system_info[cpu].FIELD.beg_parent_ip =                 \
          (AS_STRUCT(cr0_hi)).ip<<3;                            \
    }                                                           \
})


#define  store_end_ip_in_system_info(mutex,FIELD)               \
({  int  cpu;                                                   \
    register e2k_cr0_hi_t 	cr0_hi;                         \
    cpu = boot_smp_processor_id_();                             \
    AS_WORD(cr0_hi) =  E2K_GET_DSREG_NV(cr0.hi);                \
    system_info[cpu].FIELD.beg_ip = mutex->ip;                  \
    system_info[cpu].FIELD.beg_parent_ip = mutex->caller;       \
    system_info[cpu].FIELD.end_ip = E2K_GET_DSREG(ip);          \
    system_info[cpu].FIELD.end_parent_ip =                      \
     (AS_STRUCT(cr0_hi)).ip<<3;                                 \
})

#define  calculate_max_time_in_system_info(FIELD)               \
({  long t; int  cpu;                                           \
    register e2k_cr0_hi_t 	cr0_hi;                         \
    cpu = boot_smp_processor_id_();                             \
    if (enable_collect_interrupt_ticks) {                       \
        t= E2K_GET_DSREG(clkr)-system_info[cpu].                \
                                            FIELD.begin_time;   \
        system_info[cpu].FIELD.number ++;                       \
        system_info[cpu].FIELD.full_time += t;                  \
        if (system_info[cpu].FIELD.max_time < t){               \
            system_info[cpu].FIELD.max_time = t;                \
            system_info[cpu].FIELD.max_beg_ip =                 \
              system_info[cpu].FIELD.beg_ip;                    \
            system_info[cpu].FIELD.max_beg_parent_ip =          \
              system_info[cpu].FIELD.beg_parent_ip;             \
            system_info[cpu].FIELD.max_end_ip =                 \
               E2K_GET_DSREG(ip);                               \
            AS_WORD(cr0_hi) =  E2K_GET_DSREG_NV(cr0.hi);        \
            system_info[cpu].FIELD.max_end_parent_ip =          \
              (AS_STRUCT(cr0_hi)).ip<<3;                        \
            system_info[cpu].FIELD.max_begin_time =             \
                system_info[cpu].FIELD.begin_time;              \
        }                                                       \
        system_info[cpu].FIELD.begin_time = 0;                  \
    }                                                           \
})

extern long TIME;
#define  store_max_time_in_system_info(tick,FIELD)              \
({  long t; int  cpu;                                           \
    register e2k_cr0_hi_t 	cr0_hi;                         \
    cpu = boot_smp_processor_id_();                             \
    t= E2K_GET_DSREG(clkr)-tick;                                \
    if (enable_collect_interrupt_ticks) {                       \
        system_info[cpu].FIELD.number ++;                       \
        system_info[cpu].FIELD.full_time += t;                  \
      if ( system_info[cpu].FIELD.max_time < t){                \
        system_info[cpu].FIELD.max_time = t;                    \
        system_info[cpu].FIELD.max_beg_ip =                     \
              system_info[cpu].FIELD.beg_ip;                    \
        system_info[cpu].FIELD.max_beg_parent_ip =              \
              system_info[cpu].FIELD.beg_parent_ip;             \
        system_info[cpu].FIELD.max_end_ip =                     \
               E2K_GET_DSREG(ip);                               \
        AS_WORD(cr0_hi) =  E2K_GET_DSREG_NV(cr0.hi);            \
        system_info[cpu].FIELD.max_end_parent_ip =              \
              (AS_STRUCT(cr0_hi)).ip<<3;                        \
        system_info[cpu].FIELD.max_begin_time =                 \
                system_info[cpu].FIELD.begin_time;              \
      }                                                         \
      system_info[cpu].FIELD.begin_time = 0;                    \
    }                                                           \
})



#define   set_disable_interrupt_ticks(cpu, tick)                \
({                                                              \
    store_begin_time_ip_in_system_info(cpu, tick,               \
                                   max_disabled_interrupt);     \
    disable_interrupt[cpu].clk = tick;                          \
})
      
#define	UPSR_RESTORE(src_upsr)                                  \
({                                                              \
     unsigned long upsr1 =  E2K_GET_DSREG_NV(upsr);             \
     int  _cond_ = (upsr1 & UPSR_IE) != ((src_upsr) & UPSR_IE); \
     if (enable_collect_interrupt_ticks &&  _cond_) {           \
            if (src_upsr & UPSR_IE)                             \
               collect_disable_interrupt_ticks();               \
            else                                                \
               mark_disable_interrupt_ticks();                  \
     }                                                          \
     E2K_SET_UPSR_IRQ_BARRIER(src_upsr);                        \
})

#define condition_mark_disable_interrupt_ticks(_cond_)          \
({                                                              \
     if (enable_collect_interrupt_ticks) {              	\
         mark_disable_interrupt_ticks();                        \
     }                                                          \
})

#define condition_collect_disable_interrupt_ticks(_cond_)       \
({                                                              \
     if (enable_collect_interrupt_ticks && _cond_) {            \
         collect_disable_interrupt_ticks();                     \
     }                                                          \
})
 
# else /* !CONFIG_PROFILING    */

#define  store_max_time_in_system_info(tick,FIELD) 
#define  calculate_max_time_in_system_info(FIELD) 
#define  store_begin_time_in_system_info(FIELD) 
#define  store_begin_ip_in_system_info(FIELD) 
#define  info_save_tir_reg(tick)
#define  info_restore_all_reg(tick)
#define  info_save_all_reg()
#define  info_save_stack_reg(tick)
#define  info_restore_stack_reg(tick)
#define  info_save_mmu_reg(tick)
#define  info_restore_mmu_reg(tick)
#define  info_mutex(tick)
#define  info_spin_lock(tick) 
#define  switch_time()  
#define  calculate_switch_time() 
#define  soft_irq_time()    
#define  calculate_soft_irq_time()  
#define  hard_irq_time()
#define  calculate_hard_irq_time() 
#define  cpu_idle_time()
#define  calculate_cpu_idle_time()
#define  store_do_irq_ticks()
#define  define_time_of_do_irq(N)
#define condition_collect_disable_interrupt_ticks(_cond_)
#define condition_mark_disable_interrupt_ticks(_cond_)
#define collect_disable_interrupt_ticks()
#define set_disable_interrupt_ticks(cpu, tick)
#define mark_disable_interrupt_ticks()
#define init_additional_info()
#define set_calibration_result(n) 
#define add_info_syscall(n, ticks)
#define add_info_interrupt(n, ticks)
#define read_ticks(n) 
#define get_additional_size() (0)
#define	UPSR_RESTORE(src_upsr) (E2K_SET_UPSR_IRQ_BARRIER(src_upsr))
#endif /* CONFIG_PROFILING    */

/* PSR value for user threads (SGE disabled) */
#define E2K_KERNEL_PSR_ENABLED_ASM	0x73
#define E2K_KERNEL_PSR_ENABLED ((e2k_psr_t) { {	\
	pm	: 1,				\
	ie	: 1,				\
	sge	: 0,				\
	lw	: 0,				\
	uie	: 1,				\
	nmie	: 1,				\
	unmie	: 1,				\
} })

/* PSR value for user threads (SGE disabled) */
#define E2K_KERNEL_PSR_DISABLED ((e2k_psr_t) { { \
	pm	: 1,				\
	ie	: 0,				\
	sge	: 0,				\
	lw	: 0,				\
	uie	: 0,				\
	nmie	: 0,				\
	unmie	: 0,				\
} })

#ifndef	CONFIG_ACCESS_CONTROL
#define E2K_KERNEL_UPSR_ENABLED_ASM		0xa1
#define E2K_KERNEL_UPSR_DISABLED_ALL_ASM	0x01
#define E2K_KERNEL_UPSR_DISABLED ((e2k_upsr_t) { {	\
		fe	: 1, 				\
		se	: 0,				\
		ac	: 0,				\
		a20	: 0,				\
		ie	: 0,				\
		nmie	: 1				\
} })
#define E2K_KERNEL_UPSR_ENABLED ((e2k_upsr_t) { {	\
		fe	: 1, 				\
		se	: 0,				\
		ac	: 0,				\
		a20	: 0,				\
		ie	: 1,				\
		nmie	: 1				\
} })
#define E2K_KERNEL_UPSR_DISABLED_ALL ((e2k_upsr_t) { {	\
		fe	: 1,				\
		se	: 0,				\
		ac	: 0,				\
		a20	: 0,				\
		ie	: 0,				\
		nmie	: 0				\
} })
#else
#define E2K_KERNEL_UPSR_ENABLED_ASM		0xa5
#define E2K_KERNEL_UPSR_DISABLED_ALL_ASM	0x05
#define E2K_KERNEL_UPSR_DISABLED ((e2k_upsr_t) { {	\
		fe	: 1, 				\
		se	: 0,				\
		ac	: 1,				\
		a20	: 0,				\
		ie	: 0,				\
		nmie	: 1				\
} })
#define E2K_KERNEL_UPSR_ENABLED ((e2k_upsr_t) { {	\
		fe	: 1, 				\
		se	: 0,				\
		ac	: 1,				\
		a20	: 0,				\
		ie	: 1,				\
		nmie	: 1				\
} })
#define E2K_KERNEL_UPSR_DISABLED_ALL ((e2k_upsr_t) { {	\
		fe	: 1,				\
		se	: 0,				\
		ac	: 1,				\
		a20	: 0,				\
		ie	: 0,				\
		nmie	: 0				\
} })
#endif	/* ! (CONFIG_ACCESS_CONTROL) */

#define	E2K_USER_INITIAL_UPSR	((e2k_upsr_t) { {	\
		fe	: 1,				\
		se	: 0,				\
		ac	: 0,				\
		di	: 0,				\
		wp	: 0,				\
		ie	: 1,				\
		a20	: 0,				\
		nmie	: 1				\
} })

#define	E2K_USER_INITIAL_PSR	((e2k_psr_t) { {	\
		pm	: 0,				\
		ie	: 1,				\
		sge	: 1,				\
		lw	: 0,				\
		uie	: 0,				\
		nmie	: 1,				\
		unmie	: 0				\
} })


#define	INIT_KERNEL_UPSR_REG(irq_en, nmirq_dis) \
do { \
	e2k_upsr_t upsr = E2K_KERNEL_UPSR_DISABLED; \
	if (irq_en) \
		AS(upsr).ie = 1; \
	if (nmirq_dis) \
		AS(upsr).nmie = 0; \
	WRITE_UPSR_REG(upsr); \
} while (0)


#define	INIT_USER_UPSR_REG()	WRITE_UPSR_REG(E2K_USER_INITIAL_UPSR)

#define	SET_KERNEL_UPSR(disable_sge)			\
({							\
	INIT_KERNEL_UPSR_REG(false, false);		\
	SWITCH_IRQ_TO_UPSR(disable_sge);		\
})

#define SET_KERNEL_UPSR_WITH_DISABLED_NMI(disable_sge)	\
({							\
	INIT_KERNEL_UPSR_REG(false, true);		\
	SWITCH_IRQ_TO_UPSR(disable_sge);		\
})


#define	SET_USER_UPSR()					\
({							\
	RETURN_IRQ_TO_PSR();				\
	INIT_USER_UPSR_REG();				\
})

/*
 * UPSR should be saved and set to kernel initial state (where interrupts
 * are disabled) independently of trap or interrupt occurred on user
 * or kernel process.
 * In user process case it is as above.
 * In kernel process case:
 * 	Kernel process can be interrupted (so UPSR enable interrupts)
 * 	Hardware trap or system call operation disables interrupts mask
 * in PSR and PSR becomes main register to control interrupts.
 *	Trap handler should switch interrupts control from PSR to UPSR
 * previously it should set UPSR to initial state for kernel with disabled
 * interrupts (so UPSR disable interrupts)
 * If trap handler returns to trap point without UPSR restore, then
 * interrupted kernel process will have UPSR with disabled interrupts.
 * So UPSR should be saved and restored in any case
 */

#define	SWITCH_TO_KERNEL_UPSR(upsr_reg, disable_sge, irq_en, nmirq_dis)	\
({									\
	DO_SAVE_UPSR_REG((upsr_reg));					\
	INIT_KERNEL_UPSR_REG(irq_en, nmirq_dis);			\
	SWITCH_IRQ_TO_UPSR(disable_sge);				\
	if (!irq_en)							\
		trace_hardirqs_off();					\
})

/* Order is important: since trace_hardirqs_on() uses irqs_disabled()
 * which reads UPSR to check whether interrutps are really disabled,
 * it must be called before restoring UPSR. */
#ifdef CONFIG_TRACE_IRQFLAGS
# define RETURN_TO_KERNEL_UPSR(upsr_reg) \
do { \
	e2k_cr1_lo_t __cr1_lo; \
 \
	AW(__cr1_lo) = E2K_GET_DSREG_NV(cr1.lo); \
	if ((AS(__cr1_lo).psr & PSR_UIE) && AS(upsr_reg).ie \
			|| (AS(__cr1_lo).psr & PSR_IE)) \
		trace_hardirqs_on(); \
	RETURN_IRQ_TO_PSR(); \
	DO_RESTORE_UPSR_REG(upsr_reg); \
} while (0)
#else
# define RETURN_TO_KERNEL_UPSR(upsr_reg) \
do { \
	RETURN_IRQ_TO_PSR(); \
	DO_RESTORE_UPSR_REG(upsr_reg); \
} while (0)
#endif

#define	RETURN_TO_USER_UPSR(upsr_reg)		\
do {						\
	RETURN_IRQ_TO_PSR();			\
	DO_RESTORE_UPSR_REG(upsr_reg);		\
} while (0)

#ifdef	CONFIG_ACCESS_CONTROL
#define	ACCESS_CONTROL_DISABLE_AND_SAVE(upsr_to_save)	\
({ \
	e2k_upsr_t upsr; \
	upsr_to_save = read_UPSR_reg(); \
	upsr = upsr_to_save; \
	AS_STRUCT(upsr).ac = 0; \
	write_UPSR_reg(upsr); \
})

#define	ACCESS_CONTROL_RESTORE(upsr_to_restore)	\
({ \
	write_UPSR_reg(upsr_to_restore); \
})
#else /* !CONFIG_ACCESS_CONTROL */
#define	ACCESS_CONTROL_DISABLE_AND_SAVE(upsr_to_save)	do { } while (0)
#define	ACCESS_CONTROL_RESTORE(upsr_to_restore)		do { } while (0)
#endif /* CONFIG_ACCESS_CONTROL */

#define arch_align_stack(x) (x)

void default_idle(void);

extern void * __e2k_read_kernel_return_address(int n);
/* If n == 0 we can read return address directly from cr0.hi */
#define __e2k_kernel_return_address(n) \
		({ (n == 0) ? ((void *) (E2K_GET_DSREG_NV(cr0.hi) & ~7UL)) \
				: __e2k_read_kernel_return_address(n); })

typedef void (*clear_rf_t)(void);
extern const clear_rf_t clear_rf_fn[];

static __always_inline void clear_rf_kernel_except_current(u64 num_q)
{
	clear_rf_fn[num_q]();
}

#endif /* _E2K_SYSTEM_H_ */
