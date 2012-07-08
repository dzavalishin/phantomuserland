/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Unix subsystem capabilities list.
 *
 *
**/

#ifndef UUCAPAS_H
#define UUCAPAS_H

/**
 * \ingroup Unix
 * \defgroup Unix Unix compatibility subsystem
 * @{
**/

#define CAP_CHOWN		(1<<0)
#define CAP_DAC_OVERRIDE	(1<<1)
#define CAP_DAC_READ_SEARCH	(1<<2)
#define CAP_FOWNER		(1<<3)
#define CAP_FSETID		(1<<4)
#define CAP_KILL		(1<<5)
#define CAP_SETGID		(1<<6)
#define CAP_SETUID		(1<<7)
#define CAP_SETPCAP		(1<<8)
#define CAP_LINUX_IMMUTABLE	(1<<9)
#define CAP_NET_BIND_SERVICE	(1<<10)
#define CAP_NET_BROADCAST	(1<<11)
#define CAP_NET_ADMIN		(1<<12)
#define CAP_NET_RAW		(1<<13)
#define CAP_IPC_LOCK		(1<<14)
#define CAP_IPC_OWNER		(1<<15)
#define CAP_SYS_MODULE		(1<<16)
#define CAP_SYS_RAWIO		(1<<17)
#define CAP_SYS_CHROOT		(1<<18)
#define CAP_SYS_PTRACE		(1<<19)
#define CAP_SYS_PACCT		(1<<20)
#define CAP_SYS_ADMIN		(1<<21)
#define CAP_SYS_BOOT		(1<<22)
#define CAP_SYS_NICE		(1<<23)
#define CAP_SYS_RESOURCE	(1<<24)
#define CAP_SYS_TIME		(1<<25)
#define CAP_SYS_TTY_CONFIG	(1<<26)
#define CAP_HIDDEN		(1<<27)
#define CAP_INIT_KILL		(1<<28)

#endif // UUCAPAS_H

