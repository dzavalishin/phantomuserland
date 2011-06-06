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

#define phystokv(pa)	((void *)(pa))
#define kvtophys(va)	((physaddr_t)(va))


void wire_page_for_addr( void *addr, size_t count );
void unwire_page_for_addr( void *addr, size_t count );



#endif // KERNEL_VM_H
