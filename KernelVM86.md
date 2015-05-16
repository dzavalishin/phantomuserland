# Summary #

i386/vm86.c

`void phantom_v86_run(void *code, int size)` - runs given (8086) code, copying it to low (below 1MB) memory first.

Usage example: see in i386/vesa.c

# Details #

  * Runs code by switching to VM86\_TSS
  * IO access is unrestricted
  * Memory access is restriced by check\_ua()
  * VME is not used. Partially implemented, but not even tried.
  * GPF handler is overridden, so no multithreading supposed while running 86 code.

# Problems #

See phantom\_ret\_from\_vm86() - longjmp is a hack. Normally we must return by jmp to main TSS.

If you going to fix:

To check what's going on first find out which TSS is current when we get to phantom\_ret\_from\_vm86() - I suspect that it is already MAIN\_TSS.