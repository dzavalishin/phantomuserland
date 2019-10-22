# add 'set auto-load safe-path .' to ~/.gdbinit
set confirm off
symbol-file phantom.pe
symbol-file phantom
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

#break phantom_paging_init
#break hal_PIC_interrupt_dispatcher
#break arch_adjust_after_thread_switch
#break pvm_root_init

#break scr_painter.c:213
#break w_clear_control
#break w_add_control
#break find_visible_text_len

#break ctl_text_field_paint
#break ctl_text_field_events

break alloc_clear_arena


