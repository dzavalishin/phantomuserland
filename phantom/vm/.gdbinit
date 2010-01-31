set confirm off

target exec pvm_test.exe 
file pvm_test.exe 
#symbol-file pvm_test.exe 

break main
#break pvm_exec


#set debug_print_instr=1

#break pvm_exec_call
#break pvm_exec_throw

#break alloc.c:1311

break panic

break si_bootstrap_22_set_os_interface

run
