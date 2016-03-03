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
#include <kernel/init.h>
#include <kernel/mmu.h>
#include <kernel/page.h>

#include <phantom_libc.h>
#include <malloc.h>

#include <threads.h>

#include <vm/alloc.h>

#include <hal.h>



struct hardware_abstraction_level    	hal;



int phantom_is_a_real_kernel() { return 1; }




void hal_init( vmem_ptr_t va, long vs )
{

    hal.object_vspace = va;
    hal.object_vsize = vs;

    printf("HAL init: %s %s, VM @ 0x%x\n", arch_name, board_name, hal.object_vspace );
    pvm_alloc_init( va, vs );

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
    (void)getchar();
    exit(1);
}



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
    return ((addr_t)test) >= ((addr_t)hal.object_vspace) && ((addr_t)test) < ((addr_t)hal.object_vspace)+hal.object_vsize;
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




