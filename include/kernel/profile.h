/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel profiler.
 *
 *
**/

#ifndef KERNEL_PROFILER_H
#define KERNEL_PROFILER_H

#include <sys/types.h>

typedef unsigned profiler_entry_t;

// We distinguish hit addresses which differ more than this
#define PROFILER_MAP_DIVIDER 16


// size of hit map - must be > ( kernel code size / PROFILER_MAP_DIVIDER )
// curr kernel size 0x100000, reserve twice for future growth
#define PROFILER_MAP_SIZE (0x200000/PROFILER_MAP_DIVIDER)


void profiler_register_interrupt_hit( addr_t ip );
void profiler_dump_map( void );



#endif // KERNEL_PROFILER_H

