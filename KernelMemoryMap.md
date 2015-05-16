## IA32 Memory Map ##


0x0 - 0x500 - BIOS data area, preserved

0x500 - 0x100000 - low memory. Allocable except for I/O and ROM area (1mb-640k)

0x100000 - 0x??????? = 1mb - ?mb = kernel

kernel end - 0x40000000 = kernel heap, mapped as is (will be mapped not as is later)

0x40000000 - 0x80000000 = unmapped, used to allocate unbacked address space (mem mapped devices, etc)

0x80000000 - 0xC0000000 = foreground, objects space

## MIPS32 Memory Map ##

MIPS unpaged mem access address space is upper half of address space, so kernel runs in 0x80000000 and above.

Virtual machine (persistent block) is at 0x01000

Allocable address space is at 0xC0000000

## IA64 Memory Map ##

TODO. Supposedly:

0x40000000 - 0x80000000 = unmapped, used to allocate unbacked address space (mem mapped devices, etc)

0x80000000 - max positive ptr = foreground, objects space

min neg - 0 = kernel

## Problems ##

  * Videomem does not work if mapped not identically.

## Memory TODO ##


  * [KernelVM86](KernelVM86.md) is implemented and does some low mem access. Document it!
  * Page 0 - unmap to catch null ptr accesses.
  * POSIX code - CS/DS segments mapped to binary objects

## To think about ##

Interpreter code in a special segment?

## Memory control structure ##

This is how it should be, not how it is. We're implementing now what is described here.

1. Low level phys RAM allocator. Has two parts on ia32 - one is used to allocate low 1mb memory and used in a very special situations like in [KernelVM86](KernelVM86.md) support code. Main one allocates RAM which is not statically assigned to kernel image (everything above it). Allocation is in page size increments.

This allocator is used by kernel heap allocator to replentish heap, and directly by users which need page-aligned RAM (for phys-level access, usually).

2. Kernel heap allocator. Takes mem from page-level allocator, maps it above kernel end and below 0x40000000. usual malloc/free.


3. Persistent memory at 0x80000000 - 0xC0000000. Mapping to real memory (which is allocated from 1 above) and disk IO is provided by vm\_map.c. This memory belongs to main (object-level) part of OS, and is allocated/freed by VM allocator/GC. No pointer from this part can look to some other memory space.

TODO: run vm interpreter/jit code in user mode with access to lower kernel mem restricted.

4. Addres space allocator. 0x40000000 - 0x80000000 zone is pure address space (unmapped at kernel start) and is used to map memory-accessed devices (namely, APIC), video RAM (NB! - we have a problem here noww, and VRAM is mapped directly, which means it leaves in object mem space now), temp buffers, etc. Usually memory allocated from low-level page allocator is mapped here by owner.

NB! How to map device memory: use page\_map\_io parameter, see hal.h