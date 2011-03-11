/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ARM tlb flush.
 *
**/

#include <kernel/mmu.h>

/*
 * Inline function to flush a page from the TLB.
 */
void ftlbentry(physaddr_t la)
{
#warning TLB Flush!
    //asm volatile("invlpg (%0)" : : "r" (la) : "memory");
}
