# Introduction #

There are two possible Phantom userland program run environments. One is real kernel (running in qemu, usually) - this one is more or less complete. And second is user mode test environment (PhantomTestEnvironment), which currently works only in Windows (that's will change soon - Linux support is coming), and supports just a subjset of available stuff. Worst thing is that latter one does not support threads (yet?).


## Building .ph code ##

See examples in trunk/plib.

Note that existing compiler does not automatically build classes current class dependens on.