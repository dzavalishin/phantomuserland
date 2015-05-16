# Overview #

One must distinguish first boot of the given OS instance, and later ones. First boot differs because kernel has no snapshot to start from and has to build minimal working environment. Later boots are simple - find actual snapshot and map memory. Well, see details below.


# Kernel boot #


## Multiboot ##

Phantom is a multiboot kernel. Multiboot commands are like this:

```
title=phantom
kernel=(nd)/phantom -d 10 root.boot=ru.dz.phantom.tetris.boot --
module=(nd)/classes
module=(nd)/pmod_test
boot 
```

Kernel command line is:
  * kernel startup args
  * kernel environment (root.boot = boot class name, root.init = init class name, root.shell = shell class name)
  * -- separates kernel main argv/argc
  * kernel main args (unused yet)

Kernel startup args are:
```
-d=max_error_level (0-255)
-e=max_error_level (0-255)
-i=max_info_level (0-255)
-f=max_flow_level (0-255)
-s=syslog_dest_ip_addr
--pause - enables pause on pressEnter func call - for early kernel startup debug
--novesa - skip BIOS (realmode interrupt) VESA init, used to test native graphics drivers
```

Module named 'classes' (just this name) is needed during the first boot only and passes basic class set for empty OS to initialize itself. Note that these classes are not embedded into the OS state automatically - they must be explicitly requested by class loader during first run, see below.

Any module named 'pmod**_' is kernel extension - part of Unix subsystem. In the development._

Current kernel looks for Phantom formatted fs as whole disk or partition of type 0xD3 on first two IDE drives.**


## Drivers ##

See tables in driver\_map.c and kernel/ia32/board/**.c - ISA drivers are probed, PCI are started by looking up PCI config and looking up drivers by PCI codes. Drivers are started in 4 stages, se main.c**

```
    // Stage is:
    //   0 - very early in the boot - interrupts can be used only
    //   1 - boot, most of kernel infrastructure is there
    //   2 - disks which Phantom will live in must be found here
    //   3 - late and optional and slow junk
```


## Video ##

See video.c

Two paths:
  * VESA - if VESA is found, VESA driver is enforced. That's wrong, surely.
  * Else, all the videodrivers are called to probe hw and tell which resolution they can give. Biggest one is selected.

TODO: boot flag to force VESA, boot parameter to limit resolution.


## Unix subsystem ##

If compiled in, starts just before Phantom subsystem, and starts all pmod_modules in turn._

See unix/**sources.**

If FAT/FAT32 disk is found, it is mounted as /amnt[0-9] and /amntX/bin/sh is executed.

# Phantom subsystem start #

See pvm\_root\_init()

In signle thread:
  * On first start boot code is run.
  * On subsequent starts - calls restart code for border objects.

Multithread mode is started after single thread phantom code is finished.

In multithread:
  * all the rest is run.

## First run ##

See vm/root.c, pvm\_boot()

On the first run kernel 'manually' builds minimal object environment (class/int/string/... classes), loads '.ru.dz.phantom.system.boot' class (if not overriden from kernel command line), and starts VM thread to run it. All the threads run from boot code will be registered, but not started until boot code is over. Finish of the boot code lets OS to start all the VM threads.

NB: On this run root array of special references is built. These references are used by kernel on subsequent starts to reach specific objects in object space, such as list of all VM threads, etc.

Usermode phantom test program pvm\_test executes only 'first run' code now.