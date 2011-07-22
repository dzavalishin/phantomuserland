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

#define HAVE_SMP 0
#define HAVE_NET 1
#define HAVE_UNIX 1
#define HAVE_VESA 1
#define HAVE_USB 1


#define HAVE_FLOPPY 1
#define HAVE_AHCI 1

#define MEM_RECLAIM 0
// verify on-disk snapshot consistency after snapshot
#define VERIFY_SNAP 1
// verify VM consistency before snapshot
#define VERIFY_VM_SNAP 0

#define SCREEN_UPDATE_THREAD 1


#ifdef ARCH_ia32
#define HAVE_PCI 1
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

#include <string.h>

#define STRAY_CATCH_SIZE (128)

void phantom_debug_register_stray_catch( void *buf, int bufs, const char *name );

// define in each source an array of zeroes to try to catch stray pointers.
// even if we can't really catch 'em, do it so that enabling and disabling
// these arrays will, possibly, affect bugs and, if so, show us that bug is
// caused by stray pointer.
static char stray_pointer_catch_bss[STRAY_CATCH_SIZE];
static char stray_pointer_catch_data[STRAY_CATCH_SIZE] = "0000"; // enforce it to be in data seg

static void register_stray_catch_buf(void) __attribute__ ((constructor));
static void register_stray_catch_buf(void) 
{
    memset( stray_pointer_catch_data+4, 0, STRAY_CATCH_SIZE-4 );
	
    phantom_debug_register_stray_catch( stray_pointer_catch_bss, STRAY_CATCH_SIZE, " bss" );
    phantom_debug_register_stray_catch( stray_pointer_catch_data, STRAY_CATCH_SIZE, " data" );
}

#endif















#endif // ASSEMBLER


#endif // CONFIG_H
