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
#include <kernel/mmu.h>
#include <kernel/page.h>
#include <phantom_libc.h>

#include <kernel/init.h>

#include <threads.h>

// TEMP! Remove!
//#include <thread_private.h>

#include <vm/alloc.h>

//#include <x86/phantom_pmap.h>
//#include <x86/phantom_page.h>
#include <malloc.h>

//#include <i386/proc_reg.h>
//#include <i386/eflags.h>


#include <hal.h>
#include "hal_private.h"

//#include "spinlock.h"

//#define volatile /* none */


struct hardware_abstraction_level    	hal;

//static int _DEBUG = 0;


int phantom_is_a_real_kernel() { return 1; }









void hal_init( vmem_ptr_t va, long vs )
{
    printf("x86 HAL init\n");

    hal.object_vspace = va;
    hal.object_vsize = vs;

    pvm_alloc_init( va, vs );

    hal_printf("HAL init VM at 0x%X\n", hal.object_vspace);

    hal_init_vm_map();

    hal_time_init();
}

vmem_ptr_t hal_object_space_address() {
    if(hal.object_vspace == 0) panic("hal_object_space_address() - is zero yet!");
    return hal.object_vspace;
}


void hal_halt()
{
    //fflush(stderr);
    printf("\n\nhal halt called, exiting.\n");
    getchar();
    exit(1);
}


//void hal_printf( char *format, ... ) __attribte__((__deprecated__));


void hal_printf( char *format, ... )
{
    va_list argList;
    va_start(argList, format);
    vprintf( format, argList );
    va_end(argList);
    //fflush(stdout);
}

#include <sys/syslog.h>

void hal_log( char *format, ... )
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
// assert
//
// -----------------------------------------------------------------------




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




