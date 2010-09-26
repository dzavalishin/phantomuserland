set confirm off
#symbol-file phantom
#dir ../../../../userland/trunk/phantom/vm
#dir ../../../../userland/trunk/phantom/libc
#target remote localhost:1234
#dir ..

symbol-file test.exe
target exec test.exe

break main

#break phantom_threads_init

#break phantom_import_main_thread
#break phantom_thread_state_init
#break t_machdep.c:40
#break i386_fxsave

#break t_enqueue_runq
#break phantom_scheduler_select_thread_to_run

#break t_switch.c:29
#break phantom_scheduler_yield
break phantom_thread_ininterrupt_fork

run
