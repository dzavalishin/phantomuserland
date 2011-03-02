#ifndef INIT_H
#define INIT_H

// Tests


void run_test( const char *test_name, const char *test_parm );



// Startup/init                               

#include <sys/types.h>

void __phantom_run_constructors(void);

void hal_init_vm_map(void);
void phantom_timed_call_init(void);
void phantom_timed_call_init2(void);

void phantom_heap_init(void);
//void hal_init_physmem_alloc( physaddr_t start, size_t npages );
void hal_init_physmem_alloc(void);
void hal_init_physmem_alloc_thread(void);

void hal_physmem_add( physaddr_t start, size_t npages );
void hal_physmem_add_low( physaddr_t start, size_t npages );


void phantom_map_mem_equally(void);

void hal_init_vm_map(void);
void phantom_timed_call_init(void);

int detect_cpu(int curr_cpu);

void hal_cpu_reset_real(void) __attribute__((noreturn));

void phantom_threads_init(void);


void phantom_load_gdt(void);

void phantom_init_descriptors(void);
void phantom_fill_idt(void);
void phantom_load_idt(void);

void phantom_init_vm86(void);
void phantom_init_vesa(void);

void phantom_init_apic(void);

void phantom_paging_init(void);

void init_multiboot_symbols(void);

int phantom_timer_pit_init(int freq, void (*timer_intr)());

void init_irq_allocator(void);

void init_main_event_q(void);

void phantom_trfs_init(void);

void phantom_turn_off_pic_scheduler_timer(void);

void phantom_init_stat_counters(void);
void phantom_init_stat_counters2(void);


void phantom_unix_proc_init(void);

void init_buses(void);

void arch_float_init(void);

void load_classes_module(); // vm bulk classes init


#endif // INIT_H

