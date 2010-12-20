#ifndef CONFIG_H
#define CONFIG_H

#ifndef NET_CHATTY
#define NET_CHATTY 0
#endif

#define MEM_RECLAIM 0

#define COMPILE_OHCI 0
#define COMPILE_UHCI 0

#define HAVE_NET 1

// 64 Mbytes of heap for start - TODO dynamic heap growth
#define PHANTOM_START_HEAP_SIZE (1024*1024*64)

// Priority for kernel threads that supposed to react fast
//#define PHANTOM_SYS_THREAD_PRIO (THREAD_PRIO_HIGH)
#define PHANTOM_SYS_THREAD_PRIO (THREAD_PRIO_MOD_REALTIME|THREAD_PRIO_HIGH)


// Simple IDE driver has bysy loop waitng 4 inetrrupt.
// Let it give out CPU.
#define SIMPLE_IDE_YIELD 1

#endif // CONFIG_H
