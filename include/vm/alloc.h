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

#define GC_ENABLED 0


// Must be included after
#include "vm/object.h"

struct pvm_object_storage * pvm_object_alloc( unsigned int data_area_size );
//void pvm_object_delete( pvm_object_storage * );

//void pvm_alloc_init( void * _pvm_object_space_start, int size );

void pvm_alloc_clear_mem(void);


void run_gc();
void init_gc();


void gc_root_add(struct pvm_object_storage *o);
void gc_root_rm(struct pvm_object_storage *o);


#endif // PVM_ALLOC_H
