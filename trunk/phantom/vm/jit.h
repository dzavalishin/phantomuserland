#ifndef JIT_H
#define JIT_H

enum jit_callable
{
    JIT_F_LOAD_F_ACC, 			// pvm_exec_load_fast_acc
    JIT_F_THREAD_SLEEP_WORKER,          // phantom_thread_sleep_worker
    JIT_F_WAIT_SNAP, 			// phantom_thread_wait_4_snap
    JIT_F_CODE_GET_BYTE, 		// pvm_code_get_byte
    JIT_F_CREATE_OBJ, 			// pvm_create_object
    JIT_F_COPY_OBJ,                     // pvm_copy_object
    JIT_F_CREATE_INT_OBJ,               // pvm_create_int_object
};


struct jit_out
{
    // map of interpreted IP to asm instr offset

    // map of callable kernel funcs - id to func
};


typedef struct jit_out jit_out_t;

#endif // JIT_H


