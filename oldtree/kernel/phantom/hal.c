/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Supposed to be hardware abstraction layer. Mix of different things
 * in reality. Needs cleanup.
 *
**/


#include <kernel/config.h>

#include <phantom_libc.h>

#include <kernel/init.h>

#include <threads.h>

// TEMP! Remove!
#include <thread_private.h>

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

    if(mapped == page_map_io)
        bits |= INTEL_PTE_VALID|INTEL_PTE_WTHRU|INTEL_PTE_NCACHE;

    if(access == page_rw)
        bits |= INTEL_PTE_WRITE;

    pt_entry_t	pte;

    pte = create_pte(p, bits);

    if(_DEBUG) hal_printf("Mapping VA 0x%X to PA 0x%X, pte is 0x%X\n",
                          page_start_addr, p, (long)pte );

    if(mapped != page_unmap )
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









// -----------------------------------------------------------------------
//
// threads
//
// -----------------------------------------------------------------------

static void
kernel_thread_starter(void *func)
{
    void (*thread)(void) = (void (*)(void))func;
    thread();

    panic("some kernel thread is dead");
}


void*
hal_start_kernel_thread(void (*thread)(void))
{
    return phantom_create_thread( kernel_thread_starter, thread, THREAD_FLAG_KERNEL );
}











/*
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

*/







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




