set confirm off

#target exec pvm_headless.exe 
#file pvm_headless.exe 
#symbol-file pvm_headless.exe 

break main
break panic
break pvm_exec_panic
break pvm_exec_panic0

#break pvm_exec

#set debug_print_instr=1

#break _e4c_library_initialize
#break e4c_context_begin
break _e4c_library_fatal_error
break e4c_print_exception

break e4c_exception_throw_verbatim_


#break jsmn_parse
#break w_add_control
break init_task_bar

run
