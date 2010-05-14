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
#break vm_map_lazy_pageout_thread

#break phantom_bios_int_10_args

#break hal_set_thread_priority

break spinlock.c:43
break t_sleep.c:31

#break page_fault
#break vm_map_page_fault_trap_handler

#break dpc_init
#break phantom_thread_switch

break event.c:373

#set pagination off

watch pm_map->lock.lock
