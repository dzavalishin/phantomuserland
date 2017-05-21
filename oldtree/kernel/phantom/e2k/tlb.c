#include <kernel/mmu.h>

/*
 * Inline function to flush a page from the TLB.
 */

void ftlbentry(physaddr_t la)
{
#warning write me
    //asm volatile("invlpg (%0)" : : "r" (la) : "memory");
}
