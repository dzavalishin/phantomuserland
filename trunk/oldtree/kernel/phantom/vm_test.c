//#include <i386/pio.h>
#include <phantom_libc.h>


#include <hal.h>
#include "pager.h"
#include "dpc.h"
#include "vm_map.h"


#define pages 8

#define pg_one_pg 1
#define bt_one_pg 4096

int vm_pages = pages*pg_one_pg;
int vm_bytes = pages*bt_one_pg;



void vm_test()
{
    char *obj_space = (char *)vm_map_get_object_address_space_start();

    hal_printf("Test of VM memory at 0x%X\n", obj_space);

    char *addr = obj_space;

    //short data = '0';
    const char min = 0x21;
    char d1 = min, d2 = min;

    int readErrCount = 0;

#if 1
    char old_d1 = min, old_d2 = min;
    hal_printf("\n\nintegrity check of old data: ");

    while(1)
    {
        if( ! (((int)addr) % 0x800) )
        {
            putchar('?');            //fflush(stdout);
        }

        d2 = *addr++;
        d1 = *addr++;

        if( d1 != old_d1 || d2 != old_d2 )
        {
            //printf(" !! err %c%c (must be %c%c) !! ", d2, d1, old_d2, old_d1 );
            readErrCount++;
        }
        old_d1 = d1;
        old_d2 = d2;

        if( old_d1++ > 'z' )
        {
            old_d1 = min;
            if( old_d2++ > 'z' )
                old_d2 = min;
        }


        if( addr >= obj_space+vm_bytes )
            break;
    }
    printf("\n\nintegrity check done, %d errors\n\n", readErrCount);
    getchar();
#endif

#if 1
    hal_printf("Memory write: ");
    int cycle_count = 1;
    d1 = min, d2 = min;

    addr = obj_space;
    while(1)
    {
        if( ! (((int)addr) % 0x800) )
        {
            hal_printf("."); //fflush(stdout);
            //hal_sleep_msec( 55 );
        }

        *addr++ = d2;
        *addr++ = d1;

        if( d1++ > 'z' )
        {
            d1 = min;
            if( d2++ > 'z' )
                d2 = min;
        }

        if( addr >= obj_space+vm_bytes )
        {
            addr = obj_space;
            hal_printf("mem access wrap... ");
            if( --cycle_count <= 0 ) break;
        }

    }
    hal_printf(" DONE\n");
#endif
}


