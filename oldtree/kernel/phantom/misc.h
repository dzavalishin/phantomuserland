#ifndef MISC_H
#define MISC_H

// TODO move all init to kernel/init.h

#include <multiboot.h>
#include <errno.h>


// Phantom junkyard of func prototypes

// Boot state

extern struct multiboot_info bootParameters;


// Startup/init

void hal_init_vm_map(void);
void phantom_timed_call_init(void);

void phantom_heap_init(void);
//void hal_init_physmem_alloc( physaddr_t start, size_t npages );
void hal_init_physmem_alloc(void);
void hal_init_physmem_alloc_thread(void);

void hal_physmem_add( physaddr_t start, size_t npages );
void hal_physmem_add_low( physaddr_t start, size_t npages );


void phantom_map_mem_equally(void);



int main();

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






void phantom_start_video_driver(void);

int phantom_pci_find_drivers( int stage );



int detect_cpu(int curr_cpu);


// TODO move to time.h
/* wait by spinning: number of milliseconds to delay */
void phantom_spinwait(int millis);
// Uptime in seconds
time_t uptime(void);




void hal_cpu_reset_real(void);


void net_test(void);

#endif // MISC_H

// To multiboot.h

void phantom_parse_cmd_line(void);
void phantom_process_boot_options(void);
void phantom_start_boot_modules(void);



#include <multiboot.h>

struct multiboot_module *phantom_multiboot_find(const char *string);


// to softirq

#define SOFT_IRQ_THREADS        31


void hal_set_thread_name(const char *name);



errno_t load_elf( void *_elf, size_t elf_size, const char *name );

errno_t get_uldt_cs_ds(
                       linaddr_t cs_base, u_int16_t *ocs, size_t cs_limit,
                       linaddr_t ds_base, u_int16_t *ods, size_t ds_limit
                      );


void sound(u_int32_t frequency);
void nosound(void);
void beep(void);


// -----------------------------------------------------------------------
// Unix emulation
// -----------------------------------------------------------------------

extern int syscall(void);


void phantom_unix_fs_init(void);



// -----------------------------------------------------------------------
// Misc debug
// -----------------------------------------------------------------------

void check_global_lock_entry_count(void);


