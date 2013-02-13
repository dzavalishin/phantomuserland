# add 'set auto-load safe-path .' to ~/.gdbinit
set confirm off
symbol-file phantom.pe
dir .
dir ../../../phantom/vm
dir ../../../phantom/libc
dir ../../../phantom/libc/ia32
dir ../../../phantom/dev
dir ../../../phantom/libphantom 
dir ../../../phantom/newos
dir ../../../phantom/threads
dir ../../../phantom/libwin 

target remote localhost:1234

set logging file gdb.log
set logging on


break panic
break pvm_exec_panic

#break main

source -v ./.gdb-local

#watch usec_per_tick

