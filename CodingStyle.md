This page is incomplete.

## C (Kernel/VM) ##

  * Tabs: 4 spaces.
  * Doxygen brief: Java style. (Up to first dot.)

**Don't forget: you're in kernel!**

Even though currently you can run phantom virtual machine
as other operating system's process, the very same code is
compiled into the real Phantom OS kernel. It means:

  * No libc access, if library function is not in phantom/libc.
  * No pointers from object arena (phantom/vm/alloc.c pvm\_object\_space\_start/end) to any address outside of these bounds. See KernelMemoryMap.

**Document your code!**