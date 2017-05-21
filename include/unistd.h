/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Reasonable subset of unistd. Fix?
 *
 *
**/

/*
** Copyright 2001-2004 Travis Geiselbrecht. All rights reserved.
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef __newos__libc_unistd__hh__
#define __newos__libc_unistd__hh__

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* whence values for lseek() */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

void    _exit(int);
void    exit(int);

int     open(char const *, int, ...);
int     close(int);
int     dup(int);
int     dup2(int, int);
int	unlink(const char *);
int	fsync(int);
void	sync(void);

off_t   lseek(int, off_t, int);
ssize_t pread(int, void *, size_t, off_t);
ssize_t pwrite(int, void const*, size_t, off_t);

#ifdef ARCH_e2k

// elbrus compiler cries about that

int read(int, void *, size_t);
int write(int, const void *, size_t);

#else

ssize_t read(int, void *, size_t);
ssize_t write(int, void const*, size_t);

#endif

unsigned sleep(unsigned);
int      usleep(unsigned);

int   chdir(const char *);
char *getcwd(char *, size_t);
char *getwd(char *);

int	pipe(int fds[2]);

/* not strictly supposed to be here, and doesn't quite match unix ioctl() */
int	ioctl(int, int, void *, size_t);

/* process groups */
int 	setpgid(pid_t pid, pid_t pgid);
pid_t 	getpgid(pid_t pid);
int 	setpgrp(void);
pid_t 	getpgrp(void);

/* sessions */
pid_t 	getsid(pid_t pid);
pid_t 	setsid(void);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif

