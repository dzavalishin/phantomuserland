/**
 *
 * Phantom OS - Phantom kernel include file.
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal: yes
 * Preliminary: yes
 *
 *
**/

#ifndef HAL_H
#define HAL_H


#define USE_PHANTOM_PAGING_CODE 1

/**
 *
 * This header defines (very unclear and chaotic at this moment) hardware abstraction layer.
 * It is supposed (but not really achieved) that everything 'above' this layer is hardware
 * independent. 
 *
 * TODO: Bring here all the code that is different between kernel and Windows-based Phantom
 * environments.
 *
**/


#include <phantom_types.h>
#include <phantom_assert.h>
#include <errno.h>
#include <spinlock.h>

#define __MEM_GB 0x40000000u
#define __MEM_PAGE 4096
// Start of pool of virtual addresses that normally not backed with memory,
// but used to map in some physical mem page for kernel access or CPU-based IO
#define PHANTOM_AMAP_START_VADDR_POOL (__MEM_GB*1 + 0)
#define PHANTOM_AMAP_SIZE_VADDR_POOL (__MEM_PAGE*4096)

// Virtual machine lives here
//#define PHANTOM_AMAP_START_VM_POOL (__MEM_GB*2 + 0)
#define PHANTOM_AMAP_START_VM_POOL (__MEM_GB*2)



typedef enum page_mapped_t { page_unmap = 0, page_map = 1 } page_mapped_t;
typedef enum page_access_t { page_noaccess = 0, page_readonly = 1, page_readwrite = 2, page_ro = 1, page_rw = 2 } page_access_t;



struct hardware_abstraction_level
{
    vmem_ptr_t          			object_vspace;
    int             				object_vsize;
};

extern struct hardware_abstraction_level    	hal;


// Returns nonzero in real kernel and zero in win/linux test environment.
// Must not be used widely.
int 					phantom_is_a_real_kernel(void);


vmem_ptr_t 				hal_object_space_address();



void        				hal_init( vmem_ptr_t va, long vs );

void    				hal_init_object_vmem(void *start_of_virtual_address_space);


void    				hal_halt();
void    				hal_cli();
void    				hal_sti();
int                                     hal_is_sti(void); // returns true if ints enabled
int                                     hal_save_cli(void); // cli and ret 1 if was enabled

void 					phantom_mem_lock();
void 					phantom_mem_unlock();


// void    				sleep_usec( int microseconds ) = 0;
void    				hal_sleep_msec( int miliseconds );

void    				hal_printf( char *format, ... );
void        				hal_log( char *format, ... );


// paging support
static __inline__ unsigned int        	hal_min_pagesize() { return __MEM_PAGE; }
static __inline__ unsigned int        	hal_mem_pagesize() { return __MEM_PAGE; }
void *      		  		hal_paged_space(); // where paged memory starts
void        				hal_grow_paged_space( unsigned add_bytes );
static __inline__ int                 	hal_address_is_aligned( void *addr ) { return ( ((int)addr)%hal_min_pagesize() ) == 0; }



long        				hal_phys_mem_4_paging(); // how much of phys mem is available for paging at all
long        				hal_free_phys_mem_4_paging(); // how much of phys mem is available for paging is left unmapped now

void					hal_page_control( physaddr_t  p, void *page_start_addr, page_mapped_t mapped, page_access_t access );
void					hal_pages_control( physaddr_t  p, void *page_start_addr, int n_pages, page_mapped_t mapped, page_access_t access );

void	hal_page_control_etc(
                     physaddr_t  p, void *page_start_addr,
                     page_mapped_t mapped, page_access_t access,
                     u_int32_t flags
                );

void	hal_pages_control_etc( physaddr_t  pa, void *va, int n_pages, page_mapped_t mapped, page_access_t access, u_int32_t flags );


void * 					hal_alloc_page(); // allocate (identically) mapped mem page in kern addr space
void   					hal_free_page(void *page); // deallocate identically mapped page

errno_t        				hal_alloc_phys_page(physaddr_t  *result); // alloc and not map -- returns 1 on success
void        				hal_free_phys_page(physaddr_t  page); // alloc and not map - WILL PANIC if page is mapped!

errno_t        				hal_alloc_phys_pages(physaddr_t  *result, int npages); // alloc and not map
void        				hal_free_phys_pages(physaddr_t  page, int npages); // alloc and not map - WILL PANIC if page is mapped!

int         				hal_alloc_vaddress(void **result, int num); // alloc address of a page, but not memory
void        				hal_free_vaddress(void *addr, int num);

// Allocate physmem, address space for it, and map. Panics if out of anything.
void 					hal_pv_alloc( physaddr_t *pa, void **va, int size_bytes );
// Unmap, free addr space and physmem
void 					hal_pv_free( physaddr_t pa, void *va, int size_bytes );

// Low ( < 1M ) mem. Identically mapped all the time!
errno_t					hal_alloc_phys_pages_low(physaddr_t *result, int npages);
void                                    hal_free_phys_pages_low(physaddr_t  paddr, int npages);



void		hal_copy_page_v2p( physaddr_t to, void *from );
void		memcpy_p2v( void *to, physaddr_t from, size_t size );
void		memcpy_v2p( physaddr_t to, void *from, size_t size );




int                 			hal_addr_is_in_object_vmem( void *test );
void                			hal_check_addr_is_in_object_vmem( void *test );
void        				hal_register_page_fault_handler(void (*page_fault_handler)( void *address, int write ));





void        				hal_start_kernel_thread(void (*thread)(void));
void        				hal_exit_kernel_thread(void);

// returns thread id, does not check for thread death
int                                     hal_start_kernel_thread_arg(void (*thread)(void *arg), void *arg);
int					hal_start_thread(void (*thread)(void *arg), void *arg, int flags);


void                                    hal_set_thread_priority( int tid, int prio );

void hal_set_thread_name(const char *name);


void                                    hal_disable_preemption(void);
void                                    hal_enable_preemption(void);



// ------------------------------------------------------------------------------------------
// IRQ

#define HAL_IRQ_SHAREABLE 1
//#define HAL_IRQ_PRIVATE 1

int 					hal_irq_alloc( int irq, void (*func)(void *arg), void *arg, int is_shareable );
void 					hal_irq_free( int irq, void (*func)(void *arg), void *arg );

void 					hal_enable_softirq();
void 					hal_disable_softirq();


// ------------------------------------------------------------------------------------------
// Semas

struct hal_mutex
{
    struct phantom_mutex_impl *impl;
};

typedef struct hal_mutex hal_mutex_t;

errno_t					hal_mutex_init(hal_mutex_t *m, const char *name);
errno_t					hal_mutex_lock(hal_mutex_t *m);
errno_t					hal_mutex_unlock(hal_mutex_t *m);
errno_t					hal_mutex_destroy(hal_mutex_t *m);

// FIXME
#define ASSERT_LOCKED_MUTEX(m) 

struct hal_cond
{
    struct phantom_cond_impl *impl;
};

typedef struct hal_cond hal_cond_t;


errno_t 				hal_cond_init( hal_cond_t *c, const char *name );
errno_t					hal_cond_wait( hal_cond_t *c, hal_mutex_t *m );
errno_t 				hal_cond_timedwait( hal_cond_t *c, hal_mutex_t *m, long msecTimeout );
errno_t					hal_cond_signal( hal_cond_t *c );
errno_t					hal_cond_broadcast( hal_cond_t *c );
errno_t					hal_cond_destroy( hal_cond_t *c );






struct hal_sem;

typedef struct hal_sem hal_sem_t;


int 					hal_sem_init( hal_sem_t *s, const char *name );
void 					hal_sem_release( hal_sem_t *s );
int 					hal_sem_acquire( hal_sem_t *s );
void 					hal_sem_destroy( hal_sem_t *s );

#define SEM_FLAG_NO_RESCHED 1
#define SEM_FLAG_TIMEOUT 2
#define SEM_FLAG_INTERRUPTABLE 4

// TODO ERR seems to be nonworking :(
int 					hal_sem_acquire_etc( hal_sem_t *s, int val, int flags, long uSec );




// ------------------------------------------------------------------------------------------
// Timers

//typedef int64_t bigtime_t;

int 					hal_time_init(); // asks RTC

void 					hal_time_tick(int tick_rate);




// ------------------------------------------------------------------------------------------
// Debug tools


//void                			hal_dump(char *data, int len);
// void hexdump(const void *ptr, int length, const char *hdr, int flags);
// phantom_libc

extern void 				hal_assert_failed(char *file, int line);

#ifndef assert
#define assert(ex)							\
	if (!(ex))							\
		hal_assert_failed(__FILE__, __LINE__);
#endif



#endif // HAL_H
