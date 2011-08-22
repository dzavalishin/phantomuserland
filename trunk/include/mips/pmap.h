/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * MIPS asm defines. 
 *
**/

#ifndef PMAP_H
#define PMAP_H

#ifndef ARCH_mips
#warning This is MIPS code!
#endif

// Total PTE entries - just array
#define NPTE 0x100000

// Total TLB entries - TODO BUG FIXME - must be defined in board or read runtime
#define NTLBE 48

// Minimal - 4K - page size
//#define TLB_PAGEMASK 0 // - hardcoded as zero in asm


#ifndef ASSEMBLER

#include <sys/types.h>

/**
 *
 * MIPS TLB entry contains 2 adjacent pagetable entries.
 * So we need two distinct types - for TLB and for pagetable.
 * Contents are nearly the same - for TLB we double physical
 * part - 0 for low and 1 for high page.
 *
**/



// Actually, this is 8Kb entry :(
// NB! Don't change - accessed from asm
struct mips_tlb_entry
{
   u_int32_t	v;
   u_int32_t	p0;
   u_int32_t	p1;
};

typedef struct mips_tlb_entry tlb_entry_t;


//! Loads struct, returns tlbhi
int mips_tlb_read( int index, struct mips_tlb_entry *e );

//! Returns contents of Index reg after probe - top bit is failure to find TLB record
int mips_tlb_probe( struct mips_tlb_entry *e );

void mips_tlb_write_index(int index, struct mips_tlb_entry *e);
void mips_tlb_write_random( struct mips_tlb_entry *e );



struct mips_pt_entry
{
   u_int32_t	v;
   u_int32_t	p;
};

typedef struct mips_pt_entry pt_entry_t;



errno_t load_tlb_entry( addr_t va, int write );


#endif

// asm offsets to struct mips_tlb_entry
#define MIPS_PT_ENTRY_V_OFF 0
#define MIPS_PT_ENTRY_P0_OFF 4
#define MIPS_PT_ENTRY_P1_OFF 8


#define TLB_V_ADDR_SHIFT 13
#define TLB_V_ADDR_MASK 0xFFFFE000
// Or with this to get higher part of TLB mapping addr
#define TLB_V_ADDR_HI 0x1000

#define TLB_P_ADDR_MASK 0xFFFFF000
#define TLB_P_ADDR_SHIFT 6 // bits 31:12, start from bit 6 in reg

#define TLB_P_CACHE_NONE     (0x2<<2)
#define TLB_P_CACHE_WTHROUGH (0x3<<2)

#define TLB_P_DIRTY 0x4
#define TLB_P_VALID 0x2
#define TLB_P_GLOBAL 0x1

#define TLB_P_CACHE_SHIFT 3
#define TLB_P_CACHE_MASK 0x7

#define TLB_P_CACHE_UNCACHED 2
#define TLB_P_CACHE_DEFAULT 3

#define PTE_V_ADDR_MASK 0xFFFFF000


#endif // PMAP_H

