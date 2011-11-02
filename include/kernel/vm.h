/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Virtual mem macros.
 *
 *
**/

#ifndef KERNEL_VM_H
#define KERNEL_VM_H

#include <phantom_types.h>

// Phantom kernel is mapped 1:1 to physical mem.

// TEMP for coexistance with old headers
#ifndef lintokv

#define lintokv(la)	((void *)(la))
#define kvtolin(va)	((u_int32_t)(va))

#endif

#ifdef ARCH_mips
// NB! We must give out PHYSICAL addresses, and kernel is linked to 0x80000000,
// so we strip high bit from kernel address. On MIPS 0x80000000 to 0xc0000000-1 
// maps directly to 0 in kernel mode
#  define phystokv(pa)	((void *)(0x80000000 | (addr_t)(pa)))
#  define kvtophys(va)	(0x7FFFFFFF & (physaddr_t)(va))
#else
#  define phystokv(pa)	((void *)(pa))
#  define kvtophys(va)	((physaddr_t)(va))
#endif


void wire_page_for_addr( void *addr, size_t count );
void unwire_page_for_addr( void *addr, size_t count );

void vm_map_page_mark_unused( addr_t page_start);


#endif // KERNEL_VM_H
