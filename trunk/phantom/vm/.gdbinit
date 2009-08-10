set confirm off

#file pvm_test.exe 
#target exec pvm_test.exe 
#symbol-file pvm_test.exe 

#break main
#break pvm_exec

#run

#set debug_print_instr=1


#break pvm_exec_throw

break pvm_alloc.c:1297