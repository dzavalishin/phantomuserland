/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * NewOS error codes.
 *
 *
**/


/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_ERR_H
#define _NEWOS_ERR_H


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




#endif // _NEWOS_ERR_H

