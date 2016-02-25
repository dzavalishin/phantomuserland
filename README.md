Hello there.

It is a repository for Phantom OS userland code and cross-development tools.
de-facto it contains kernel sources as well, though.

Code, contained here, is not all we have for Phantom - we upload only
those parts of code which are more or less ready to be co-developed.

Basically, most of code we (the original team) upload is very straighforward
and, sometimes, dumb. That, to some extent, is on purpose. We want to have
a working system first and polish it next. Besides, not all the concepts and
design desicions are final, so it is of no use to finalize all the 
implementation desisions now as well.

Code is known to compile successfully with cygwin/linux gcc 4.3.4.
Only ia32 target is most complete and stable, arm port is in active development
but very instable, mips port is just started - compiles and can breath for a
second :), amd64 port is incomplete and does not compile at all.



BUILD

  "make all" in trunk

  In Windows you will need Cygwin to do that. http://www.cygwin.com
  Select, at least: gcc4, subversion, binutils, make, gdb
  (see etc/cygwin_get.cmd)

RUN

  Run phantom.cmd/phantom.sh in trunk/run
  See doc/RUNNING for more details

DEBUG

  Run QEMU (see above) and then - gdb in trunk/oldtree/kernel/phantom

  Kernel console is logged to trunk/run/serial0.log 

  Kernel is able to send logging info to syslogd by UDP.
  Currently syslogd address is hardcoded in net_misc.c.

DIRECTORIES

trunk/oldtree/kernel/phantom - kernel 
trunk/phantom                - libs and unix userland (user/apps)
trunk/plib/sys/src           - native phantom userland code

trunk/run                    - QEMU run/test environment
trunk/plc                    - phantom language compiler / java bytecode translator

DOX

  https://github.com/dzavalishin/phantomuserland/wiki
  doc/*

SCREENSHOTS

  https://github.com/dzavalishin/phantomuserland/wiki/ScreenShots

[![Code coverage][COVERAGE_BADGE]][COVERAGE_LINK]

[COVERAGE_LINK]:
https://scan.coverity.com/projects/dzavalishin-phantomuserland

[COVERAGE_BADGE]:
https://scan.coverity.com/projects/8024/badge.svg


Best regards, Dmitry Zavalishin,
Phantom project... how to say... inventor? :)
dz@dz.ru
