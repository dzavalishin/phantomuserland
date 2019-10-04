/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/

#ifndef PVM_EXEC_H
#define PVM_EXEC_H

// __dead2
#include <sys/cdefs.h>
#include <vm/internal_da.h>

// Size of syscall instruction in bytes, to rewind IP on execution syscall
#define VM_SYSCALL_INSTR_SIZE 2


void pvm_exec(pvm_object_t current_thread);

void pvm_exec_panic0( const char *reason ) __dead2;
void pvm_exec_panic( const char *reason, struct data_area_4_thread *da ) __dead2;

//! Load current thread data to fast access copy fields in thread object data area
void pvm_exec_load_fast_acc(struct data_area_4_thread *da);

//! Save current thread data from fast access copy fields in thread object data area to actual places. In fact just IP is saved.
void pvm_exec_save_fast_acc(struct data_area_4_thread *da);


pvm_object_t  pvm_exec_find_method( pvm_object_t o, unsigned method_index, struct data_area_4_thread *tda );
void pvm_exec_set_cs( struct data_area_4_call_frame* cfda, pvm_object_t  code );


pvm_object_t 
pvm_exec_run_method(
                    pvm_object_t this_object,
                    int method,
                    int n_args,
                    pvm_object_t args[]
                   );


void create_and_run_object(const char *class_name, int method );


typedef struct dynamic_method_info
{
    pvm_object_t        new_this;
    pvm_object_t        target_class;
    pvm_object_t        method_name;

    int                 method_ordinal;
    int                 n_param;
} dynamic_method_info_t;

#endif // PVM_EXEC_H


