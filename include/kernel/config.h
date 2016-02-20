/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Configuration. Turn on/off kernel parts.
 *
 *
**/

#ifndef CONFIG_H
#define CONFIG_H

// Use new partitioning functions (using handles) - UNFINISHED
#define CONF_NEW_PART_FUNC 1

// Use new semaphores functions (using handles) - UNFINISHED
#define CONF_NEW_SEMA_FUNC 0


// vm class instanceof checks for parents
#define VM_INSTOF_RECURSIVE 1
#define VM_DEFERRED_REFDEC 0

#define OLD_VM_SLEEP 0
#define NEW_SNAP_SYNC 0


#define COMPILE_PERSISTENT_STATS 1

// TODO killme
#define NEW_WINDOWS 0

#define VM_UNMAP_UNUSED_OBJECTS 0

#define ATA_32_PIO 1

#ifdef ARCH_ia32
#  define HAVE_SMP 0
#  define HAVE_NET 1
#  define HAVE_UNIX 1
#  define HAVE_VESA 1

#  define HAVE_FLOPPY 1
#  define HAVE_AHCI 0
#  define HAVE_KOLIBRI 1
#else
#  define HAVE_SMP 0
#  define HAVE_NET 0
#  define HAVE_UNIX 0
#  define HAVE_VESA 0

#  define HAVE_FLOPPY 0
#  define HAVE_AHCI 0
#  define HAVE_KOLIBRI 0
#endif

#define MEM_RECLAIM 1
// verify on-disk snapshot consistency after snapshot
#define VERIFY_SNAP 1
// verify VM consistency before snapshot
#define VERIFY_VM_SNAP 0

#define SCREEN_UPDATE_THREAD 1


#ifdef ARCH_ia32
#define HAVE_PCI 1
#define HAVE_USB 1
#endif


#ifndef HAVE_PCI
#define HAVE_PCI 0
#endif



#if !defined(NO_STRAY_CHECK)
# define HAVE_STRAY 0
#endif

#define DRIVE_SCHED_FROM_RTC 0



#ifndef COMPILE_EXPERIMENTAL
#define COMPILE_EXPERIMENTAL 0
#endif  // COMPILE_EXPERIMENTAL

#  define PAGING_PARTITION 1

#if COMPILE_EXPERIMENTAL
#  define PAGING_PARTITION 1
#  define USE_ONLY_INDIRECT_PAINT 1
#endif


#ifndef NET_CHATTY
#define NET_CHATTY 0
#endif


#define COMPILE_OHCI 0
#define COMPILE_UHCI 0


#define COMPILE_WEAKREF 0



// 64 Mbytes of heap for start - TODO dynamic heap growth
#define PHANTOM_START_HEAP_SIZE (1024*1024*64)

// Priority for kernel threads that supposed to react fast
//#define PHANTOM_SYS_THREAD_PRIO (THREAD_PRIO_HIGH)
#define PHANTOM_SYS_THREAD_PRIO (THREAD_PRIO_MOD_REALTIME|THREAD_PRIO_HIGH)

// Simple IDE driver has busy loop waitng 4 inetrrupt.
// Let it give out CPU.
#define SIMPLE_IDE_YIELD 1




#ifndef ASSEMBLER

#if HAVE_STRAY
#include <kernel/stray.h>
#endif


#endif // ASSEMBLER


#endif // CONFIG_H
