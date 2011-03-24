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

target remote localhost:1234

set logging file gdb.log
set logging on

source -v .gdb-local

break panic
break pvm_exec_throw

break main

break phantom_switch_context
