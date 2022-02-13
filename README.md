# Welcome to Phantom OS #

Phantom OS is a persistent operating system. Its primary goal is to provide
environment for programs that survive OS reboot. Such an environment greatly
simplifies software development and makes it possible to write programs that
for example do not need a filesystem.

Nevertheless, Phantom supports filesystems and all of the modern OS features.

For the details please refer to the [Phantom OS Programmer's guide](https://phantomdox.readthedocs.io/en/latest/).

For the questions and discussion please visit [chat](https://gitter.im/PhantomOS/HowTo) or [Google Group](https://groups.google.com/forum/?pli=1#!forum/phantom-os).

## What Phantom OS is ##

* [Phantom OS Programmer's guide](https://phantomdox.readthedocs.io/en/latest/)
* [Phantom Architecture in English](https://github.com/dzavalishin/phantomuserland/wiki/PhantomArchitecture)
* [Short article in English - TheRegister](http://www.theregister.co.uk/2009/02/03/phantom_russian_os/)
* [Big article in Russian - Open Systems Magazine](http://www.osp.ru/os/2011/03/13008200/)

Basically, most of code we (the original team) upload is very straighforward
and, sometimes, dumb. That, to some extent, is on purpose. We want to have
a working system first and polish it next. Besides, not all the concepts and
design desicions are final, so it is of no use to finalize all the 
implementation desisions now as well.

Code is known to compile successfully with cygwin/linux gcc 4.3.4. Only the
IA32 target is most complete and stable, the ARM port is in active development
but very unstable, the MIPS port has just been started - compiles and can
breathe for a second :), the amd64 port is incomplete and does not compile at all.

## Current state

See the [changelog](https://github.com/dzavalishin/phantomuserland/wiki/ChangeLog),
have a look at [screenshots](https://github.com/dzavalishin/phantomuserland/wiki/ScreenShots).
Here is the last one:

![](https://github.com/dzavalishin/phantomuserland/blob/master/doc/images/Phantom_screen_Controls_21_10_2019.png?raw=true)

More screenshots: <https://github.com/dzavalishin/phantomuserland/wiki/ScreenShots>

## Docs

[![Documentation Status](https://readthedocs.org/projects/phantomdox/badge/?version=latest)](https://phantomdox.readthedocs.io/en/latest/?badge=latest)
     
[Web Documentation](https://phantomdox.readthedocs.io/en/latest/) and [PDF](https://buildmedia.readthedocs.org/media/pdf/phantomdox/latest/phantomdox.pdf), and there's a lot of info in the [Wiki](https://github.com/dzavalishin/phantomuserland/wiki)

## Building

Set the `PHANTOM_HOME` environment variable to the path of the Phantom repository root directory
and `make all` there.

On Windows you will need [Cygwin](http://www.cygwin.com) to do that. Select at least: `gcc4`, `subversion`, `binutils`, `make`, `gdb`
(see `etc/cygwin_get.cmd`)
  
See also [TOOLCHAIN](https://github.com/dzavalishin/phantomuserland/blob/master/TOOLCHAIN)

## Running

Run phantom.cmd/phantom.sh in `/run`
  
See [doc/RUNNING](doc/RUNNING) for more details

## Debugging

Run QEMU (see above) and then - gdb in `/oldtree/kernel/phantom`

Kernel console is logged to `/run/serial0.log`

Kernel is able to send logging info to syslogd by UDP.
Currently syslogd address is hardcoded in net_misc.c.

## Directories

* `oldtree/kernel/phantom` - kernel 
* `phantom`                - libs and unix userland (user/apps)
* `plib/sys/src`           - native phantom userland code
* `run`                    - QEMU run/test environment
* `tools/plc`              - phantom language compiler / java bytecode translator

## Go on and take part!

[How to take part](https://github.com/dzavalishin/phantomuserland/wiki/HowToTakePart)

### The kernel and the compiler

Creating an unusual operating system is a very interesting thing to do. There are challenges on each and every step.
Just to start with:

* Persistent memory **garbage collector**. Suppose we're in a 64 bit world and persistent memory size is some 20 Tb. The current GC is incomplete.
* If we touch memory too much, the snapshot engine will spend a lot of IO to store the difference. A fast and good **allocator** can reduce IO load. There is one, but it could be better.
* There's need for a fast alpha-blending **image transfer (bitblt)** code.
* The Unix subsystem is very limited. There is no **signal delivery**, for example. It waits for the one who will implement the missing parts.
* It is theoretically possible to implement a **persistent Unix environment**. Quite challenging.
* Drivers - current set is minimal, The **AHCI driver** is not complete, **USB needs optimization**, some more must be ported or written.
* It would be interesting to add a **Python frontend** to the Phantom compiler. Are you a Python fan? Can help?
* Phantom bytecode supports classes, inheritance, but does not support interfaces. It is not really trivial to **implement interfaces** in an efficient way.
* Even a **simple JIT engine** will help a lot.
* The **TCP stack** is not ideal and needs someone to lend a hand.

### Porting Phantom

Ports to **ARM** and **MIPS** were started, but long time no progress. I'm looking for one who can help with that.

Bringing it to **64 bit Intel/AMD** is actual task too. 

Current kernel is basically SMP ready, but **SMP support** is not finished completely.

### Userland

There's a need to implement **demo applications for Phantom** - even simple ones will help.

More serious task is to bring in some simple **HTML renderer** and get a basic browser working.

### Build and CI

* There is a need for a **good CI setup** which can run system in a specific configurations and following special scenarios.
* Bytecode engine needs to be tested with **random garbage execution**.
* It is a good idea to keep a **set of tools** that for sure build a correct OS kernel. `cc`, `binutils`, `qemu`, etc.
* Need setup to build an **ISO image** of the system to run on different machines and emulators.
* There is a real need to do **CI on a real hadrware**. Need corresponding scripts.

If you feel interested to take part in the project, please leave me a note. An issue on a GitHub is the ideal communications channel.

[Issues to start with](https://github.com/dzavalishin/phantomuserland/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)

## Badges ##

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/8eec7d75d73b4a93b45a1befa3b70696)](https://www.codacy.com/manual/dzavalishin/phantomuserland?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=dzavalishin/phantomuserland&amp;utm_campaign=Badge_Grade)

[![Build Status](https://travis-ci.com/dzavalishin/phantomuserland.svg?branch=master)](https://travis-ci.com/dzavalishin/phantomuserland)

[![Code coverage][COVERAGE_BADGE]][COVERAGE_LINK]

[COVERAGE_LINK]:https://scan.coverity.com/projects/dzavalishin-phantomuserland

[COVERAGE_BADGE]:https://scan.coverity.com/projects/8024/badge.svg

## Communications ##

The easiest way is to [Write AN e-mail to Dmitry Zavalishin](mailto:dz@dz.ru)

Or leave a message in the [Google group](https://groups.google.com/forum/#!forum/phantom-os)

Or leave an issue in the [tracker](https://github.com/dzavalishin/phantomuserland/issues)

There is a [Phantom Website](http://phantomos.org/), but since you're here you
better look at the [Wiki](https://github.com/dzavalishin/phantomuserland/wiki).

<hr>

Best regards, Dmitry Zavalishin,
<dz@dz.ru>
