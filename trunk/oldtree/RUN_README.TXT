BUILD

Make all libs (incl kernel/phantom/threads, kernel/phantom/i386)
Make in kernel/phantom

RUN

Run phantom.cmd in run11.
Or try even more recent qemu.

DEBUG

Just run phantom.cmd in run11 and then - gdb in kernel/phantom


Kernel is able to send logging info to syslogd by UDP.
Currently syslogd address is hardcoded in net_misc.c.
