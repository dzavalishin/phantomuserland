/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes?
 *
 *
 **/

#ifndef PVM_ALLOC_H
#define PVM_ALLOC_H

#include "vm/object.h"


#define GC_ENABLED 0

// gc

void run_gc();
void init_gc();

void gc_root_add(struct pvm_object_storage *o);
void gc_root_rm(struct pvm_object_storage *o);


// allocator

// Make sure this object won't be deleted with refcount dec
// used on sys global objects
void ref_saturate_p(pvm_object_storage_t *p);
void ref_saturate_o(pvm_object_t o);

void ref_dec_o(pvm_object_t o);
void ref_inc_o(pvm_object_t o);


pvm_object_storage_t * pvm_object_alloc( unsigned int data_area_size );
//void pvm_object_delete( pvm_object_storage_t * );


void pvm_alloc_init( void * _pvm_object_space_start, unsigned int size );
void pvm_alloc_clear_mem(void);

pvm_object_storage_t *get_root_object_storage();

int pvm_memcheck();
bool pvm_object_is_allocated(pvm_object_storage_t *p);
void pvm_object_is_allocated_assert(pvm_object_storage_t *p);


#endif // PVM_ALLOC_H
