#ifndef KERNEL_VM_H
#define KERNEL_VM_H

#include <phantom_types.h>

// Phantom kernel is mapped 1:1 to physical mem.

// TEMP for coexistance with old headers
#ifndef lintokv

#define lintokv(la)	((void *)(la))
#define kvtolin(va)	((u_int32_t)(va))

#define phystokv(pa)	((void *)(pa))
#define kvtophys(va)	((physaddr_t)(va))

#endif

void wire_page_for_addr( void *addr );
void unwire_page_for_addr( void *addr );



#endif // KERNEL_VM_H
