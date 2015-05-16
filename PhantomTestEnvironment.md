# Phantom Test Environment #

The pvm\_test.exe program can run Phantom code in usual Windows environment. (Btw, port to unix/X11 is welcome.) It takes the same 'bulk' class bundle as a Phantom kernel and starts code as if it was started by a new OS instance.

Todo:
  * It would be great to have GDB networked interface to be able to debug Phantom code with GDB.
  * Due to this program internal RGBA bitmap representation is, really, BGRA, which is plain wrong. Win32 display 'driver' has to be redone.
  * It would be good to start adding networking to Phantom starting from here. Setup TCP/UDP classes and connect directly to host OS implementation?

# Differences #

pvm\_test is different from real kernel:
  * It has no threads. (yet?)
  * Not all of kernel interfaces are available (see .internal class sources for details)
  * It has no snapshots. Though simply saving object space to file can be added fairly easily.

See INSTALL file in trunk to find out how to setup environment.