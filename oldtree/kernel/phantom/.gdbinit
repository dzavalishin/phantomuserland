set confirm off
symbol-file phantom
dir .
dir ../../../phantom/vm
dir ../../../phantom/libc
dir ../../../phantom/dev
target remote localhost:1234

set logging file gdb.log
set logging on

break panic
break pvm_exec_throw

break main

source -v .gdb-local
