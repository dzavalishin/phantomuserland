# Welcome to Phantom OS #

Hello there.

It is a repository for Phantom OS userland code and cross-development tools.
de-facto it contains kernel sources as well, though.

## What Phantom OS is ##

*   [Phantom Architecture in English](https://github.com/dzavalishin/phantomuserland/wiki/PhantomArchitecture)
*   [Short article in English - TheRegister](http://www.theregister.co.uk/2009/02/03/phantom_russian_os/)
*   [Big article in Russian - Open Systems Magazine](http://www.osp.ru/os/2011/03/13008200/)

Basically, most of code we (the original team) upload is very straighforward
and, sometimes, dumb. That, to some extent, is on purpose. We want to have
a working system first and polish it next. Besides, not all the concepts and
design desicions are final, so it is of no use to finalize all the 
implementation desisions now as well.

Code is known to compile successfully with cygwin/linux gcc 4.3.4.
Only ia32 target is most complete and stable, arm port is in active development
but very instable, mips port is just started - compiles and can breath for a
second :), amd64 port is incomplete and does not compile at all.

[How to take part](https://github.com/dzavalishin/phantomuserland/wiki/HowToTakePart)

## Current state ##

See [ChangeLog](https://github.com/dzavalishin/phantomuserland/wiki/ChangeLog), look at [ScreenShots](https://github.com/dzavalishin/phantomuserland/wiki/ScreenShots), and here is the last one:

![](https://github.com/dzavalishin/phantomuserland/blob/master/doc/images/Phantom_screen_Controls_21_10_2019.png?raw=true)

## Dox ##

[![Documentation Status](https://readthedocs.org/projects/phantomdox/badge/?version=latest)](https://phantomdox.readthedocs.io/en/latest/?badge=latest)
     
[Web Documentation](https://phantomdox.readthedocs.io/en/latest/) and [PDF](https://buildmedia.readthedocs.org/media/pdf/phantomdox/latest/phantomdox.pdf)

## Build ##

  Set '''PHANTOM_HOME''' environment variable to the path to Phantom repository root directory,
  "make all" there.

  In Windows you will need Cygwin to do that. <http://www.cygwin.com>
  Select, at least: gcc4, subversion, binutils, make, gdb
  (see etc/cygwin_get.cmd)
  
  See also [TOOLCHAIN](https://github.com/dzavalishin/phantomuserland/blob/master/TOOLCHAIN)

## Run ##

  Run phantom.cmd/phantom.sh in trunk/run
  See doc/RUNNING for more details

## Debug ##

  Run QEMU (see above) and then - gdb in trunk/oldtree/kernel/phantom

  Kernel console is logged to trunk/run/serial0.log 

  Kernel is able to send logging info to syslogd by UDP.
  Currently syslogd address is hardcoded in net_misc.c.

## Directories ##

trunk/oldtree/kernel/phantom - kernel 
trunk/phantom                - libs and unix userland (user/apps)
trunk/plib/sys/src           - native phantom userland code

trunk/run                    - QEMU run/test environment
trunk/tools/plc              - phantom language compiler / java bytecode translator

## Dox ##

  <https://github.com/dzavalishin/phantomuserland/wiki>
  doc/*

## Screenshots ##

  <https://github.com/dzavalishin/phantomuserland/wiki/ScreenShots>

## Go on and take part ##

### Kernel and compiler

Creating an unusual operating system is a very interesting thing to do. There are challenges on each and every step.
Just to start with:

* Persistent memory garbage collector. Suppose we're in a 64 bit world and persistent memory size is some 20 Tb. Current GC is incomplete.
* If we touch memory too much snapshot engine will spend a lot of IO to store difference. Fast and good allocator can reduce IO load. There is one, but it could be better.
* There's need for a fast alpha-blending image transfer (bitblt) code.
* Unix subsystem is very limited. No signal delivery, for example. It waits for one who will implement missing parts.
* It is theoretically possible to implement a persistent Unix environment. Quite challenging.
* Drivers - current set is minimal, AHCI driver is not complete, USB needs optimization, some more must be ported or written.
* It would be interesting to add Python frontend to Phantom compiler. Are you a Python fan? Can help?
* Phantom bytecode supports classes, inheritance, but does not support interfaces. It is not really trivial to implement interfaces in an efficient way.
* Even simple JIT engine will help a lot.
* TCP stack is not ideal and needs someone to lend a hand.

### Porting Phantom

Ports to ARM and MIPS were started, but long time no progress. I look for one who can help with that.

Bringing it to 64 bit Intel/AMD is actual task too. 

Current kernel is basically SMP ready, but SMP support is not finished completely.

### Userland

There's need to implement demo applications for Phantom - even simple ones will help.

More serious task is to bring in some simple HTML renderer and get basic browser working.

### Build and CI

* There is a need for a good CI setup which can run system in a specific configurations and following special scenarios.
* Bytecode engine needs to be tested with random garbage execution.
* It is a good idea to keep set of tools that for sure build correct OS kernel. cc/binutils/qemu, etc.
* Need setup to build ISO image of system to run on different machines and emulators.
* There is real need to do CI on a real hadrware. Need corresponding scripts.

If you feel interested to take part in a project, please leave me a note. An issue on a GitHub is ideal communications channel.

[Issues to start with](https://github.com/dzavalishin/phantomuserland/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)

## Badges ##

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/8eec7d75d73b4a93b45a1befa3b70696)](https://www.codacy.com/manual/dzavalishin/phantomuserland?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=dzavalishin/phantomuserland&amp;utm_campaign=Badge_Grade)

[![Build Status](https://travis-ci.com/dzavalishin/phantomuserland.svg?branch=master)](https://travis-ci.com/dzavalishin/phantomuserland)

[![Code coverage][COVERAGE_BADGE]][COVERAGE_LINK]

[COVERAGE_LINK]:https://scan.coverity.com/projects/dzavalishin-phantomuserland

[COVERAGE_BADGE]:https://scan.coverity.com/projects/8024/badge.svg

## Communications ##

Easiest way is to [Write e-mail to Dmitry Zavalishin](mailto:dz@dz.ru)

Just for reference there was [Old Google forum](https://groups.google.com/forum/#!forum/phantom-os)

Here is an [Phantom Web Site](http://phantomos.org/), but since you're here you
better look at [Wiki](https://github.com/dzavalishin/phantomuserland/wiki).

<hr>

Best regards, Dmitry Zavalishin,
Phantom project... how to say... inventor? :)
dz@dz.ru
