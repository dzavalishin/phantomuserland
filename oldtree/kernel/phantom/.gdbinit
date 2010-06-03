set confirm off
symbol-file phantom
dir .
dir ../../../../userland/trunk/phantom/vm
dir ../../../../userland/trunk/phantom/libc
dir ../../../../userland/trunk/phantom/dev
target remote localhost:1234

set logging file gdb.log
set logging on

break panic
break pvm_exec_throw

break main

break vm_map.c:1174
break vm_map.c:1183
break vm_map.c:1191
break vm_map.c:1201

