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

#break vm_map.c:1174
#break vm_map.c:1183
#break vm_map.c:1191
#break vm_map.c:1201

#break drv_video_window_receive_event


#break page_fault
#break vm_map_page_fault_trap_handler

break t_kill_thread
break t_do_kill_thread
break t_do_some_kills
