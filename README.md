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

See [ChangeLog](https://github.com/dzavalishin/phantomuserland/wiki/ChangeLog), look at [ScreenShots](https://github.com/dzavalishin/phantomuserland/wiki/ScreenShots), and kere is the last one:

![](https://github.com/dzavalishin/phantomuserland/blob/master/doc/images/phantom_screen_08_10_2019_new_UI.png?raw=true)

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
