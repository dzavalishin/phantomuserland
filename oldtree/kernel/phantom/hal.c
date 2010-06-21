/**
 *
 *
 * 
 *
 *
**/

#include "config.h"

#include <phantom_libc.h>

#include <kernel/init.h>

#include <threads.h>

// TEMP! Remove!
#include "threads/thread_private.h"

#include <vm/alloc.h>

#include <x86/phantom_pmap.h>
#include <x86/phantom_page.h>
#include <malloc.h>

#include <i386/proc_reg.h>
#include <i386/eflags.h>


#include "hal.h"
#include "hal_private.h"

#include "spinlock.h"

#define volatile /* none */


struct hardware_abstraction_level    	hal;

static int _DEBUG = 0;


int phantom_is_a_real_kernel() { return 1; }







/*
 * Inline function to flush a page from the TLB.
 */
static __inline__
void ftlbentry(physaddr_t la)
{
    asm volatile("invlpg (%0)" : : "r" (la) : "memory");
}





// Used by kernel memory allocator to guard multithreaded allocation access
//static hal_spinlock_t hal_mem_lock;

//void phantom_mem_lock() { hal_spin_lock(&hal_mem_lock); }
//void phantom_mem_unlock() { hal_spin_unlock(&hal_mem_lock); }



void        hal_init( vmem_ptr_t va, long vs )
{
    printf("x86 HAL init\n");

    hal.object_vspace = va;
    hal.object_vsize = vs;

    //hal_spin_init(&hal_mem_lock);

    pvm_alloc_init( va, vs );

    //hal.object_vsize = 40 * 1024 * 1024; // 40 MB
    //hal.object_vspace = start_of_virtual_address_space;
    //hal.object_vspace = (void *)PHANTOM_AMAP_START_VM_POOL;

    hal_printf("HAL init VM at 0x%X\n", hal.object_vspace);

    hal_init_vm_map();

    hal_time_init();
}

vmem_ptr_t hal_object_space_address() {
    if(hal.object_vspace == 0) panic("hal_object_space_address() - is zero yet!");
    return hal.object_vspace;
}


void    hal_halt()
{
    //fflush(stderr);
    printf("\n\nhal halt called, exiting.\n");
    getchar();
    exit(1);
}




void        hal_printf( char *format, ... )
{
    va_list argList;
    va_start(argList, format);
    vprintf( format, argList );
    va_end(argList);
    //fflush(stdout);
}

#include <sys/syslog.h>

void        hal_log( char *format, ... )
{
    va_list argList;
    va_start(argList, format);
    //vfprintf( stderr, format, argList );
    vsyslog(LOG_KERN, format, argList);

    va_end(argList);
    //fflush(stderr);
}




// -----------------------------------------------------------------------
//
// memory
//
// -----------------------------------------------------------------------



void
hal_init_object_vmem(void *start_of_virtual_address_space)
{
    (void) start_of_virtual_address_space;
}



int
hal_addr_is_in_object_vmem( void *test )
{
    return ((int)test) >= ((int)hal.object_vspace) && ((int)test) < ((int)hal.object_vspace)+hal.object_vsize;
}

void
hal_check_addr_is_in_object_vmem( void *test )
{
    if( !hal_addr_is_in_object_vmem( test ) )
        panic("address not in object arena range");
}






void
hal_page_control(
                 physaddr_t  p, void *page_start_addr,
                 page_mapped_t mapped, page_access_t access
                )
{
    hal_page_control_etc( p, page_start_addr, mapped, access, 0 );
}



void
hal_page_control_etc(
                     physaddr_t  p, void *page_start_addr,
                     page_mapped_t mapped, page_access_t access,
                     u_int32_t flags
                )
{
    assert(PAGE_ALIGNED(p));
    assert(PAGE_ALIGNED((unsigned)page_start_addr));
    assert((flags & INTEL_PTE_PFN) == 0);

    if(mapped != page_map) access = page_noaccess;

    int bits = INTEL_PTE_USER | flags; // We need it for V86 mode - REDO IN A MORE SPECIFIC WAY, so that only VM86 pages are user accessible

    if(mapped == page_map)
        bits |= INTEL_PTE_VALID;

    if(access == page_rw)
        bits |= INTEL_PTE_WRITE;

    pt_entry_t	pte;

    pte = create_pte(p, bits);

    if(_DEBUG) hal_printf("Mapping VA 0x%X to PA 0x%X, pte is 0x%X\n",
                          page_start_addr, p, (long)pte );

    if(mapped == page_map)
        phantom_map_page( (linaddr_t)page_start_addr, pte );
    else
        phantom_unmap_page( (linaddr_t)page_start_addr );
    ftlbentry((int)page_start_addr);
}



void
hal_pages_control( physaddr_t  pa, void *va, int n_pages, page_mapped_t mapped, page_access_t access )
{
    while( n_pages-- )
    {
        hal_page_control( pa, va, mapped, access );
        pa += 4096;
        va += 4096;
    }
}

void
hal_pages_control_etc( physaddr_t  pa, void *va, int n_pages, page_mapped_t mapped, page_access_t access, u_int32_t flags )
{
    while( n_pages-- )
    {
        hal_page_control_etc( pa, va, mapped, access, flags );
        pa += 4096;
        va += 4096;
    }
}







//void * 		hal_alloc_page() { return smemalign(hal_mem_pagesize(),hal_mem_pagesize()); }
//void   		hal_free_page(void *page) { sfree(page,hal_mem_pagesize()); }





// -----------------------------------------------------------------------
//
// threads
//
// -----------------------------------------------------------------------

static void
kernel_thread_starter(void *func)
{
    //hal_printf("Starting thread at 0x%X", func );

    void (*thread)(void) = (void (*)(void))func;
    thread();

    panic("some kernel thread is dead");
}


void*
hal_start_kernel_thread(void (*thread)(void))
{
    //phantom_thread_t *t =
    return phantom_create_thread( kernel_thread_starter, thread, THREAD_FLAG_KERNEL );
}



/*
void
hal_exit_kernel_thread(void)
{
    //panic("can't kill some kernel thread");
    t_kill_thread( GET_CURRENT_THREAD()->tid );
}
*/









void
hal_dump(char *data, int len)
{
    int i;
    for( i = 0; i < len; i++)
    {
        if( (i != 0) && (0 == (i % 64)) )
            printf("\n");
        int c = *data;
        if( c < ' ' ) c = '.';
        //printf("%c.%02X ", c, *data++ );
        printf("%c", *data++ );
    }
}







#if !USE_NEW_SEMAS

// -----------------------------------------------------------------------
//
// semaphores - used by newos code, mostly?
//
// -----------------------------------------------------------------------



//typedef struct hal_sem hal_sem_t;


int hal_sem_init( hal_sem_t *s, const char *name )
{
    return hal_cond_init( &(s->c), name ) || hal_mutex_init( &(s->m), name );
}

void hal_sem_release( hal_sem_t *s )
{
    hal_mutex_lock( &(s->m) );
    s->posted++;
    hal_cond_broadcast( &(s->c) );
    hal_mutex_unlock( &(s->m) );
}


int hal_sem_acquire( hal_sem_t *s )
{
    if( hal_mutex_lock( &(s->m) ) )
        return -1;

    s->posted--;
    while(s->posted < 0)
        hal_cond_wait( &(s->c), &(s->m) );
    hal_mutex_unlock( &(s->m) );
    return 0;
}


void hal_sem_destroy( hal_sem_t *s )
{
    hal_cond_destroy( &(s->c) );
    hal_mutex_destroy( &(s->m) );
}

int hal_cond_timedwait( hal_cond_t *c, hal_mutex_t *m, long msecTimeout );


// TODO ERR seems to be nonworking :(
int hal_sem_acquire_etc( hal_sem_t *s, int val, int flags, long uSec )
{
    assert(!(flags & ~SEM_FLAG_TIMEOUT));

    if( hal_mutex_lock( &(s->m) ) )
        return -1;

    s->posted -= val;
    while(s->posted < 0)
    {
        if(flags & SEM_FLAG_TIMEOUT )
            hal_cond_timedwait( &(s->c), &(s->m), ((uSec-1)/1000)+1 );
        else
            hal_cond_wait( &(s->c), &(s->m) );
    }
    hal_mutex_unlock( &(s->m) );
    return 0;
}

#endif





void hal_assert_failed(char *file, int line)
{
    printf("Assert failed at %s:%d", file, line );
    _exit(33);
}



// -----------------------------------------------------------------------
//
// end of HAL impl
//
// -----------------------------------------------------------------------




