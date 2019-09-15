set confirm off

target exec pvm_headless.exe 
file pvm_headless.exe 
#symbol-file pvm_headless.exe 

break main
#break pvm_exec


#set debug_print_instr=1

#break pvm_exec_call
#break pvm_exec_throw

#break alloc.c:1311

break panic

#break pvm_boot

#break pvm_create_interface_object

#break _e4c_library_initialize
#break e4c_context_begin
break _e4c_library_fatal_error
break e4c_print_exception

break e4c_exception_throw_verbatim_

#break pvm_exec_sys

run
