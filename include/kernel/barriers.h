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
#define mem_barrier()

#define mem_write_barrier()
#define mem_read_barrier() mem_barrier()

// make sure caches across CPUs are coherent
#define	smp_mem_barrier()

#endif

#ifdef ARCH_arm

#define mem_barrier() arm_mem_barrier()

#define mem_write_barrier() arm_mem_write_barrier()
#define mem_read_barrier()  arm_mem_read_barrier()


#endif



#ifndef mem_barrier
#error no barrier definiton for this arch
#endif

#ifndef smp_mem_barrier
// make sure caches across CPUs are coherent
#error SMP no barrier definiton for this arch
#endif

