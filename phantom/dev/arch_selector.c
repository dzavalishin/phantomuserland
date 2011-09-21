#if 0
/*
 ** Copyright 2002, Michael Noisternig. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */

#include <newos/compat.h>
#include <ia32/selector.h>

#include <ia32/seg.h>
#include <hal.h>
#include <kernel/atomic.h>

//#include <boot/arch/i386/stage2.h>
//#include <kernel/arch/cpu.h>
//#include <kernel/smp.h>
//#include <kernel/int.h>
//#include <kernel/arch/i386/selector.h>

#define MAX_SELECTORS GDTSZ
//(GDT_LIMIT/8)
#define ENTRIES (MAX_SELECTORS/(sizeof(uint32)*8))

static int32_t selector_bitmap[ENTRIES]
    = { 0x000000ff };  // first 8 selectors reserved

static selector_type *gdt_table;
static struct gdt_idt_descr descr = { GDT_LIMIT - 1, 0 };

void i386_selector_init( void *gdt )
{
    gdt_table = (selector_type *)gdt;
    descr.b = (unsigned int *)gdt_table;
}

// creates a new selector in the gdt of given type (use SELECTOR macro)
// IN:	selector type
// RET:	selector that can be directly used for segment registers
//		0 on error
selector_id i386_selector_add( selector_type type )
{
    static hal_spinlock_t spinlock;
    int state;
    uint32 mask;
    selector_id id = 0;
    unsigned i;

    state = hal_save_cli();
    acquire_spinlock( &spinlock );

    for ( i = 0; i < ENTRIES; i++ )
        if ( selector_bitmap[i] != 0xffffffff ) {  // found free place in there
            id = i*sizeof(uint32)*8;
            mask = 1;
            while ( selector_bitmap[i] & mask ) {
                mask <<= 1;
                id++;
            }
            selector_bitmap[i] |= mask;
            gdt_table[id] = type;
            break;
        }

    release_spinlock( &spinlock );
    if( state ) hal_sti();

    if ( id ) {
        asm("lgdt	%0;"
            : : "m" (descr));
    }

    return id*8;
}

// removes a selector with given id from the gdt
void i386_selector_remove( selector_id id )
{
    if ( id < 8*8 || id >= MAX_SELECTORS*8 )
        return;

    id /= 8;
    gdt_table[id] = 0;

    atomic_and( &selector_bitmap[id/32], ~(1<<(id&31)) );

    asm("lgdt	%0;"
        : : "m" (descr));
}

// returns the selector type of a given id
selector_type i386_selector_get( selector_id id )
{
    return gdt_table[id/8];
}

#endif

