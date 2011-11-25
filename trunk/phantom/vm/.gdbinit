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

break pvm_boot

#break ev_get_unused
#break ev_q_put_win
#break ev_put_event

#break init_main_event_q

run
