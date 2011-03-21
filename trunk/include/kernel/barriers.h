/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Memory access barriers.
 *
 *
**/

#ifdef ARCH_ia32
// None on x86 PC

#define mem_write_barrier()
#define mem_barrier()
#endif




#ifndef mem_barrier
#error no barrier definiton for this arch
#endif

