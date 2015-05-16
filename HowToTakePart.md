Very good start is to do some code review for the part you want to work on, and, possibly, to write some regression tests for it.

Some possible tasks:

  * AHCI driver
  * virtio disk/net drivers
  * USB HC drivers
  * ARM port, MIPS port

All of that is partially done, and needs to be either finished or fixed.

General directions to move:

  * drivers: as usual, we need more and more of them. Some drivers exist and have to be tuned/fixed, some need to be written. Disk io, USB, video accelerators are good examples. Non-PC drivers are welcome as well.

  * kernel: lot of stuff is not final or not ideal. See issues tab or contact dz.

  * virtaul machine: it is quite obvious how to put libjit in, and it has to be done. There's quite hard refcount cleanup task as well. Full scale offline mark/sweeep on disk GC has to be written too.

  * bytecode translator: jvm to phantom bytecode translator is partially made but is not complete.

  * Dox, including general code comments.

  * PC test setup: we really need regular tests on real hardware.

  * Arm/MIPS ports - QEMU tests, real hardware tests, drivers, debugging, etc.