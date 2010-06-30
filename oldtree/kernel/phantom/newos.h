/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_H
#define _NEWOS_H

#include <phantom_types.h>


// definitions to import newos code easily

//typedef unsigned long addr_t;
//typedef unsigned int ssize_t;
typedef int thread_id;



#ifndef offsetof
#define offsetof(type, member) ((size_t)&(((type *)0)->member))
#endif



#define false 0
#define true 0xFF


#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#else
#warning PAGE_SIZE defined twice
#endif

#define PAGE_ALIGN(x) (((x) + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1))


#define system_time hal_system_time




#define dprintf printf
#define kmalloc malloc
#define kfree free

#define ASSERT assert





#define sem_id hal_sem_t
#define sem_delete(s) hal_sem_destroy(&(s))
#define sem_release(s) hal_sem_release(&(s))
#define sem_acquire(s) hal_sem_acquire(&(s))

#define sem_acquire_etc(s, v, fl, tm, xx ) hal_sem_acquire_etc( &(s), v, fl, tm )

#define mutex_init(m,name) hal_mutex_init(m)
#define mutex_lock hal_mutex_lock
#define mutex_unlock hal_mutex_unlock
#define mutex hal_mutex_t
#define mutex_destroy hal_mutex_destroy

#define acquire_spinlock hal_spin_lock
#define release_spinlock hal_spin_unlock

#define int_disable_interrupts()  int __newos_intstate = hal_save_cli()
#define int_restore_interrupts()  if(__newos_intstate) hal_sti()


#define thread_create_kernel_thread(n,f,a) hal_start_kernel_thread_arg(f,a)
#define thread_set_priority(t,pri) hal_set_thread_priority( t, pri )

// TODO BUG! Hardcode!
#define THREAD_MAX_RT_PRIORITY 31

#define thread_kill_thread_nowait(tid) panic("thread_kill_thread_nowait at %s:%d",__FILE__,__LINE__)

#define thread_snooze(nsec) hal_sleep_msec((nsec)/1000)






#define uint32 u_int32_t
#define uint16 u_int16_t
#define uint8 u_int8_t




#define CHECK_BIT(a, b) ((a) & (1 << (b)))
#define SET_BIT(a, b) ((a) | (1 << (b)))
#define CLEAR_BIT(a, b) ((a) & (~(1 << (b))))

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))


#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))
#define ROUNDOWN(a, b) (((a) / (b)) * (b))









#ifndef __ASSEMBLY__

#if (__GNUC__ == 3) || (__GNUC__ == 4)
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define _PACKED __attribute__((packed))
#define _ALIGNED(x) __attribute__((aligned(x)))

#endif // #ifndef __ASSEMBLY__








#define NO_ERROR 0

/* General errors */
#define ERR_GENERAL              (-1)
#define ERR_NO_MEMORY            (ERR_GENERAL-1)
#define ERR_IO_ERROR             (ERR_GENERAL-2)
#define ERR_INVALID_ARGS         (ERR_GENERAL-3)  // do not change, used in kernel/arch/i386/arch_interrupts.S
#define ERR_TIMED_OUT            (ERR_GENERAL-4)
#define ERR_NOT_ALLOWED          (ERR_GENERAL-5)
#define ERR_PERMISSION_DENIED    (ERR_GENERAL-6)
#define ERR_INVALID_BINARY       (ERR_GENERAL-7)
#define ERR_INVALID_HANDLE       (ERR_GENERAL-8)
#define ERR_NO_MORE_HANDLES      (ERR_GENERAL-9)
#define ERR_UNIMPLEMENTED        (ERR_GENERAL-10)
#define ERR_TOO_BIG              (ERR_GENERAL-11)
#define ERR_NOT_FOUND            (ERR_GENERAL-12)
#define ERR_NOT_IMPLEMENTED      (ERR_GENERAL-13)
#define ERR_OUT_OF_RANGE         (ERR_GENERAL-14)
#define ERR_BAD_SYSCALL          (ERR_GENERAL-15)
#define ERR_INTERRUPTED          (ERR_GENERAL-16)

/* Semaphore errors */
#define ERR_SEM_GENERAL          (-1024)
#define ERR_SEM_DELETED          (ERR_SEM_GENERAL-1)
#define ERR_SEM_TIMED_OUT        (ERR_SEM_GENERAL-2)
#define ERR_SEM_OUT_OF_SLOTS     (ERR_SEM_GENERAL-3)
#define ERR_SEM_NOT_ACTIVE       (ERR_SEM_GENERAL-4)
#define ERR_SEM_NOT_INTERRUPTABLE (ERR_SEM_GENERAL-5)
#define ERR_SEM_NOT_FOUND        (ERR_SEM_GENERAL-6)

/* Tasker errors */
#define ERR_TASK_GENERAL         (-2048)
#define ERR_TASK_PROC_DELETED    (ERR_TASK_GENERAL-1)
#define ERR_TASK_THREAD_KILLED   (ERR_TASK_GENERAL-2)

/* VFS errors */
#define ERR_VFS_GENERAL          (-3072)
#define ERR_VFS_INVALID_FS       (ERR_VFS_GENERAL-1)
#define ERR_VFS_NOT_MOUNTPOINT   (ERR_VFS_GENERAL-2)
#define ERR_VFS_PATH_NOT_FOUND   (ERR_VFS_GENERAL-3)
#define ERR_VFS_INSUFFICIENT_BUF (ERR_VFS_GENERAL-4)
#define ERR_VFS_READONLY_FS      (ERR_VFS_GENERAL-5)
#define ERR_VFS_ALREADY_EXISTS   (ERR_VFS_GENERAL-6)
#define ERR_VFS_FS_BUSY          (ERR_VFS_GENERAL-7)
#define ERR_VFS_FD_TABLE_FULL    (ERR_VFS_GENERAL-8)
#define ERR_VFS_CROSS_FS_RENAME  (ERR_VFS_GENERAL-9)
#define ERR_VFS_DIR_NOT_EMPTY    (ERR_VFS_GENERAL-10)
#define ERR_VFS_NOT_DIR          (ERR_VFS_GENERAL-11)
#define ERR_VFS_WRONG_STREAM_TYPE   (ERR_VFS_GENERAL-12)
#define ERR_VFS_ALREADY_MOUNTPOINT (ERR_VFS_GENERAL-13)
#define ERR_VFS_IS_DIR           (ERR_VFS_GENERAL-14)

/* VM errors */
#define ERR_VM_GENERAL           (-4096)
#define ERR_VM_INVALID_ASPACE    (ERR_VM_GENERAL-1)
#define ERR_VM_INVALID_REGION    (ERR_VM_GENERAL-2)
#define ERR_VM_BAD_ADDRESS       (ERR_VM_GENERAL-3)
#define ERR_VM_PF_FATAL          (ERR_VM_GENERAL-4)
#define ERR_VM_PF_BAD_ADDRESS    (ERR_VM_GENERAL-5)
#define ERR_VM_PF_BAD_PERM       (ERR_VM_GENERAL-6)
#define ERR_VM_PAGE_NOT_PRESENT  (ERR_VM_GENERAL-7)
#define ERR_VM_NO_REGION_SLOT    (ERR_VM_GENERAL-8)
#define ERR_VM_WOULD_OVERCOMMIT  (ERR_VM_GENERAL-9)
#define ERR_VM_BAD_USER_MEMORY   (ERR_VM_GENERAL-10)

/* Elf errors */
#define ERR_ELF_GENERAL		  (-5120)
#define ERR_ELF_RESOLVING_SYMBOL  (ERR_ELF_GENERAL-1)

/* Ports errors */
#define ERR_PORT_GENERAL          (-6144)
#define ERR_PORT_DELETED          (ERR_PORT_GENERAL-1)
#define ERR_PORT_OUT_OF_SLOTS     (ERR_PORT_GENERAL-2)
#define ERR_PORT_NOT_ACTIVE       (ERR_PORT_GENERAL-3)
#define ERR_PORT_CLOSED	          (ERR_PORT_GENERAL-4)
#define ERR_PORT_TIMED_OUT        (ERR_PORT_GENERAL-5)
#define ERR_PORT_NOT_FOUND        (ERR_PORT_GENERAL-6)

/* Net errors */
#define ERR_NET_GENERAL           (-7168)
#define ERR_NET_FAILED_ARP        (ERR_NET_GENERAL-1)
#define ERR_NET_BAD_PACKET        (ERR_NET_GENERAL-2)
#define ERR_NET_ARP_QUEUED        (ERR_NET_GENERAL-3)
#define ERR_NET_NO_ROUTE          (ERR_NET_GENERAL-4)
#define ERR_NET_SOCKET_ALREADY_BOUND (ERR_NET_GENERAL-5)
#define ERR_NET_ALREADY_CONNECTED (ERR_NET_GENERAL-6)
#define ERR_NET_CONNECTION_REFUSED (ERR_NET_GENERAL-7)
#define ERR_NET_NOT_CONNECTED     (ERR_NET_GENERAL-8)
#define ERR_NET_REMOTE_CLOSE      (ERR_NET_GENERAL-9)
#define ERR_NET_NOT_LISTENING     (ERR_NET_GENERAL-10)
#define ERR_NET_BAD_ADDRESS       (ERR_NET_GENERAL-11)

/* Pipe errors */
#define ERR_PIPE_GENERAL          (-8192)
#define ERR_PIPE_WIDOW            (ERR_PIPE_GENERAL-1)

/* Device errors */
#define ERR_DEV_GENERAL           (-9216)
#define ERR_DEV_TIMED_OUT         (ERR_DEV_GENERAL-1)
#define ERR_DEV_HARDWARE_ERROR    (ERR_DEV_GENERAL-2)
#define ERR_DEV_BUSY              (ERR_DEV_GENERAL-3)


enum {
	INT_NO_RESCHEDULE,
	INT_RESCHEDULE
};


#endif // _NEWOS

