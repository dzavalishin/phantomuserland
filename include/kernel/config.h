#ifndef CONFIG_H
#define CONFIG_H

// 64 Mbytes of heap for start - TODO dynamic heap growth
#define PHANTOM_START_HEAP_SIZE (1024*1024*64)

// Priority for kernel threads that supposed to react fast
//#define PHANTOM_SYS_THREAD_PRIO (THREAD_PRIO_HIGH)
#define PHANTOM_SYS_THREAD_PRIO (THREAD_PRIO_MOD_REALTIME|THREAD_PRIO_HIGH)


#endif // CONFIG_H
