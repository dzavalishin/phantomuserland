![http://misc.dz.ru/~dz/phantom/Phantom_Structure.png](http://misc.dz.ru/~dz/phantom/Phantom_Structure.png)

## General ##

Phantom is, basically, a virtual machine (VM) working in a huge persistent
virtual memory. Part of the VM classes (some classes, called 'internal') are
implemented in kernel, giving VM code access to low level kernel services.
Persistent virtual memory is completely orthogonal to object space and VM
(no relation between, for example, object boundary and virtual memory page,
etc.) and is implemented so that abrupt computer failure or loss of power
leaves system in coherent state. On the application code (VM bytecode) level
OS shutdown (either manual or caused by failure) is not 'seen' - applications
and their data are 'never die', they continue their work after the next OS
boot up as if no shutdown ever happened.

## Persistent memory and snapshots ##

Phantom does regular snapshot of the whole virtual memory. Snapshots are
done asycnchronously and without stopping the world, but resulting snapshot
is synchronous - all the memory is being snapped at the very same moment in
OS's 'personal' time. It means that snapshot state is captured like if all
the system was stopped, dumped to disk and then run again. But without stopping.


## Virtual machine ##


Phantom bytecode is traditional stack-machine bytecode. It is very like
Java bytecode, but there is no difference between built-in and user
types on the method boundary level. Any object is class instance. There is
some specific support for integer calculations though, to speedup things.

It is supposed that Phantom bytecode can represent any Java program, and
Java to Phantom bytecode convertor is being written. Other languages are
supposed to be brought through the Java bytecode or directly, by writing
specific language frontend.

C# (CLR) bytecode convertor is planned, but no work is in progress yet.


## Kernel communication ##

There is set of (internal) classes, which methods are implemented in kernel.
These classes offer kernel interface in object environment. Unlike Java,
in Phantom all the class code is either interbal (native, in Java terms), or
bytecode-level. Paranoia, yeah.

## Drivers ##

Currently drivers are written in C and live in the kernel completely.
Future releases will have possibility to write drivers in userland by
providing required kernel frameworks. Userland driver will be restricted
to communicate with given (by kernel) set of hardware resources only, and
its interrupt-handling method will be guaranteed to not to be paged out.


## Native code ##

Hi-performance code (number-crunching, video or sound processing, etc)
requires good low-level access to the processor and memory. It is supposed
that Phantom will be able to run native code in a special binary-object-bound
thread. One binary object will provide code (CS content), others will be
available as DS/SS and, possibly, ES/FS/GS (on other architectures there objects
will be just mapped in thread address space). It will let Phantom to
execute highly-optimised native code.

It is supposed that to support some portability it will be possible to provide
not exactly native, but LLVM-level code for this environment.

## POSIX environment ##

Based on the native code subsystem it is possible to build POSIX-like
environment to support existing Unix applications.