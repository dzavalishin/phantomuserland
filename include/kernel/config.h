#ifndef CONFIG_H
#define CONFIG_H

#ifndef COMPILE_EXPERIMENTAL
#define COMPILE_EXPERIMENTAL 0
#endif  // COMPILE_EXPERIMENTAL

#ifndef NET_CHATTY
#define NET_CHATTY 0
#endif

#define MEM_RECLAIM 0

#define COMPILE_OHCI 0
#define COMPILE_UHCI 0

#define HAVE_NET 1
#define HAVE_UNIX 0

// 64 Mbytes of heap for start - TODO dynamic heap growth
#define PHANTOM_START_HEAP_SIZE (1024*1024*64)

// Priority for kernel threads that supposed to react fast
//#define PHANTOM_SYS_THREAD_PRIO (THREAD_PRIO_HIGH)
#define PHANTOM_SYS_THREAD_PRIO (THREAD_PRIO_MOD_REALTIME|THREAD_PRIO_HIGH)


// Simple IDE driver has busy loop waitng 4 inetrrupt.
// Let it give out CPU.
#define SIMPLE_IDE_YIELD 1


#if 0
// define in each source an array of zeroes to try to catch stray pointers.
// even if we can't really catch 'em, do it so that enabling and disabling
// these arrays will, possibly, affect bugs and, if so, show us that bug is
// caused by stray pointer.
static char stray_pointer_catch_bss[4096];
static char stray_pointer_catch_data[4096] = "0000"; // enforce it to be in data seg

static void register_stray_catch_buf(void) __attribute__ ((constructor));
static void register_stray_catch_buf(void) 
{
	(void) stray_pointer_catch_bss;
	(void) stray_pointer_catch_data;
}

#endif


#endif // CONFIG_H
