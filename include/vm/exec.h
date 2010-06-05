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


#include "vm/internal_da.h"


void pvm_exec(struct pvm_object current_thread);

void pvm_exec_throw( const char *reason );
void pvm_exec_load_fast_acc(struct data_area_4_thread *da);

struct pvm_object_storage * pvm_exec_find_method( struct pvm_object o, unsigned method_index );
void pvm_exec_set_cs( struct data_area_4_call_frame* cfda, struct pvm_object_storage * code );


struct pvm_object
pvm_exec_run_method(
                    struct pvm_object this_object,
                    int method,
                    int n_args,
                    struct pvm_object args[]
                   );


#endif // PVM_EXEC_H


