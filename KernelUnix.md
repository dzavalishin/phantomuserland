# Unix in Phantom #

Phantom has (quite limited for now) Unix subsystem. It is supposed to evolve to three different kinds of things:

  1. Kernel modules. Loaded by bootloader, started by kernel during startup before any object stuff is run. Supposed to be used as drivers and kernel extensions. Nevertheless, run in usermode, in separate (but organized with segments, not CR3) address space, and have access to Unix syscalls. **That is what we have working already.**
  1. Unix processes, nonpersistent. Half-step to next bullet. More or less usual Unix processes. Difference from above is that they're started later and with fork/exec, as usual. The only thing which is not ready for this is fork/exec syscalls.
  1. Unix processes, persistent. Same as previous, but living in persisted object space. For this to be implemented the problem of disconnecting/reconnecting to kernel has to be solved. Example: what to do with process blocked in read syscall? Generally to have persistent Unix process we need to have persistant kernel environment.

# Goal #

Main reason to have Unix subsystem is to support Unix applications. That means than syscalls and overall support will be dictated by app's requirements.

# Implementation details #

ELF32 x86 executables loaded at address 0 (zero). Own startup and libc. No dynlinking yet.

## Address space ##

Unix process has separate AS, which is included in global Phantom address space with the help of i386 segments. It makes copyin/copyout and address translation to be trivial, but has its own problems, like inability to grow (sbrk()) without movind all the process addr space.

## Syscalls ##

```
#include <kernel/syscalls.h>
#include <kernel/unix.h>
#include <unix/uufile.h>
#include <unix/uuprocess.h>
#include <unix/uucapas.h>
```

Call gate. Params on stack, ret in eax, errno in edx.
Basic IO (open/r/w/close/ioctl) is done, more to come.



# Todo #

## Unix itself ##

The most needed things to be done ASAP are:

  * sbrk
  * end of DS/brk guard page
  * stack guard page, stack grow
  * above the stack guard page (to catch copyout runaways)
  * fork/exec/argv
  * exit!! :)
  * signals

## Phantom integration ##

The main thing to implement is ability to call object's methods. One of solutions is to have object FS which will let Phantom to "open" objects as files. For binary objects or objects representing data streams usual red/write/mmap syscalls will be usable. For all objects some special _method call_ syscall has to be implemented, which has file (object!) descriptors, strings and ints as parameters and returns new file (object!) descriptor for returned object. Any other idea?

Second is direct access to binary/string (byte stream) object contents. Possible solutions:

  1. mapping to a segment, use of FS/GS - easy, but intel-specific
  1. mapping with mmap to process AS. much better and is in sync with object access via file descriptors