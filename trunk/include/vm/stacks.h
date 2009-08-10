/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/

#ifndef PVM_STACKS_H
#define PVM_STACKS_H


#include "vm/internal_da.h"

void pvm_ostack_push( struct data_area_4_object_stack* stack, struct pvm_object o );
struct pvm_object pvm_ostack_pop( struct data_area_4_object_stack* stack );
struct pvm_object pvm_ostack_top( struct data_area_4_object_stack* stack );
int pvm_ostack_empty( struct data_area_4_object_stack* stack );

struct pvm_object  pvm_ostack_pull( struct data_area_4_object_stack* stack, int pos );

void pvm_ostack_abs_set( struct data_area_4_object_stack* stack, int pos, struct pvm_object o );
struct pvm_object pvm_ostack_abs_get( struct data_area_4_object_stack* stack, int pos );

void pvm_istack_abs_set( struct data_area_4_integer_stack* rootda, int abs_pos, int val );
int pvm_istack_abs_get( struct data_area_4_integer_stack* rootda, int abs_pos );



void pvm_istack_push( struct data_area_4_integer_stack* stack, int o );
int pvm_istack_pop( struct data_area_4_integer_stack* stack );
int pvm_istack_top( struct data_area_4_integer_stack* stack );
int pvm_istack_empty( struct data_area_4_integer_stack* stack );



void pvm_estack_push( struct data_area_4_exception_stack* stack, struct pvm_exception_handler e );
struct pvm_exception_handler pvm_estack_pop( struct data_area_4_exception_stack* stack );
struct pvm_exception_handler pvm_estack_top( struct data_area_4_exception_stack* stack );
int pvm_estack_empty( struct data_area_4_exception_stack* stack );

int pvm_estack_foreach(
                       struct data_area_4_exception_stack* stack,
                       void *pass,
                       int (*func)( void *pass, struct pvm_exception_handler *elem ));


#endif // PVM_STACKS_H
