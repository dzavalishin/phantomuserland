#include "SYS.h"


SYSCALL(_exit)
SYSCALL(ssyslog)
SYSCALL(getpagesize)
SYSCALL(personality)
SYSCALL(uname)
SYSCALL(sync)
SYSCALL(sysinfo)
SYSCALL(sysfs)
SYSCALL(klogctl)
SYSCALL(shutdown)
SYSCALL(reboot)



SYSCALL(sleepmsec)
SYSCALL(nanosleep)
SYSCALL(time)
SYSCALL(getitimer)
SYSCALL(setitimer)
SYSCALL(gettimeofday)



SYSCALL(brk)
SYSCALL(fork)
SYSCALL(vfork)
SYSCALL(nice)
SYSCALL(ioctl)
SYSCALL(kill)
//SYSCALL(pipe)
SYSCALL(umask)
SYSCALL(clone)

SYSCALL(wait)
SYSCALL(waitpid)

