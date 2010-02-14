#include <mach/asm.h>
#include "SYS.h"

SYSCALL(gethostname)
SYSCALL(sethostname)

SYSCALL(socket)
SYSCALL(bind)
SYSCALL(accept)
SYSCALL(connect)
SYSCALL(listen)
SYSCALL(recv)
SYSCALL(recvfrom)
SYSCALL(recvmsg)
SYSCALL(send)
SYSCALL(sendto)
SYSCALL(sendmsg)
SYSCALL(sendfile)
SYSCALL(setsockopt)
SYSCALL(socketcall)
SYSCALL(socketpair)
SYSCALL(getpeername)
SYSCALL(getsockname)
SYSCALL(getsockopt)

