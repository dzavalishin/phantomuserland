/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Functions, defined in architecture/machine dependent way.
 *
 *
**/

#include <sys/types.h>

void arch_cpu_invalidate_TLB_list(addr_t pages[], int num_pages);
void arch_cpu_invalidate_TLB_range(addr_t start, addr_t end);

addr_t arch_get_fault_address(void);

void *arch_get_frame_pointer();
