/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Access to VM stacks: object, binary and exception handler
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/

#ifndef PVM_STACKS_H
#define PVM_STACKS_H


#include "vm/internal_da.h"

void                pvm_ostack_push( struct data_area_4_object_stack* stack, pvm_object_t o );
pvm_object_t   pvm_ostack_pop( struct data_area_4_object_stack* stack );
pvm_object_t   pvm_ostack_top( struct data_area_4_object_stack* stack );
int                 pvm_ostack_empty( struct data_area_4_object_stack* stack );

// push nulls to reserve stack space
void                pvm_ostack_reserve( struct data_area_4_object_stack* rootda, int n_slots );


pvm_object_t   pvm_ostack_pull( struct data_area_4_object_stack* stack, int pos );

void                pvm_ostack_abs_set( struct data_area_4_object_stack* stack, int pos, pvm_object_t o );
pvm_object_t   pvm_ostack_abs_get( struct data_area_4_object_stack* stack, int pos );

// Return number of elements in stack
int                 pvm_ostack_count( struct data_area_4_object_stack* rootda );


void                pvm_istack_abs_set( struct data_area_4_integer_stack* rootda, int abs_pos, int val );
int                 pvm_istack_abs_get( struct data_area_4_integer_stack* rootda, int abs_pos );

void                pvm_lstack_abs_set( struct data_area_4_integer_stack* rootda, int abs_pos, int64_t val );
int64_t             pvm_lstack_abs_get( struct data_area_4_integer_stack* rootda, int abs_pos );


void                pvm_istack_push( struct data_area_4_integer_stack* stack, int o );
int                 pvm_istack_pop( struct data_area_4_integer_stack* stack );
int                 pvm_istack_top( struct data_area_4_integer_stack* stack );
int                 pvm_istack_empty( struct data_area_4_integer_stack* stack );

// push 0 to reserve stack space
void pvm_istack_reserve( struct data_area_4_integer_stack* rootda, int n_slots );

void                pvm_lstack_push( struct data_area_4_integer_stack* rootda, int64_t o );
int64_t             pvm_lstack_pop( struct data_area_4_integer_stack* rootda );
int64_t             pvm_lstack_top( struct data_area_4_integer_stack* rootda );



void                pvm_estack_push( struct data_area_4_exception_stack* stack, struct pvm_exception_handler e );
struct pvm_exception_handler    pvm_estack_pop( struct data_area_4_exception_stack* stack );
struct pvm_exception_handler    pvm_estack_top( struct data_area_4_exception_stack* stack );
int                 pvm_estack_empty( struct data_area_4_exception_stack* stack );

int                 pvm_estack_foreach(
                               struct data_area_4_exception_stack* stack,
                               void *pass,
                               int (*func)( void *pass, struct pvm_exception_handler *elem ));


#endif // PVM_STACKS_H
