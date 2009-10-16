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

#include <hal.h>
#include "vm/object.h"





// allocator

pvm_object_storage_t * pvm_object_alloc( unsigned int data_area_size, unsigned int flags, bool saturated );
//void pvm_object_delete( pvm_object_storage_t * );


void pvm_alloc_init( void * _pvm_object_space_start, unsigned int size );
void pvm_alloc_clear_mem();

pvm_object_storage_t *get_root_object_storage();

int pvm_memcheck();
bool pvm_object_is_allocated_light(pvm_object_storage_t *p);
bool pvm_object_is_allocated(pvm_object_storage_t *p);
void pvm_object_is_allocated_assert(pvm_object_storage_t *p);



// gc

void run_gc();

// Make sure this object won't be deleted with refcount dec
// used on sys global objects
void ref_saturate_o(pvm_object_t o);

pvm_object_t  ref_dec_o(pvm_object_t o);
pvm_object_t  ref_inc_o(pvm_object_t o);



// ------------------------------------------------------------
// shared between alloc.c and gc.c


// Gigant lock for now. TODO
extern hal_mutex_t  alloc_mutex;

void * get_pvm_object_space_start();
void * get_pvm_object_space_end();

void refzero_process_children( pvm_object_storage_t *o );
void ref_saturate_p(pvm_object_storage_t *p);




// Free'd object
#define PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE 0x00

// This is an allocated object
#define PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED 0x01

// This object has zero reference count, but objects it references are not yet
// processed. All the children refcounts must be decremented and then this object
// can be freed.
#define PVM_OBJECT_AH_ALLOCATOR_FLAG_REFZERO 0x02



// Flags for cycle detection candidates (noninternal objects only),
// - can be joined with 0x00, 0x01 or 0x02
//
// this one set when refcounter goes down to nonzero value, and clears otherwise
#define PVM_OBJECT_AH_ALLOCATOR_FLAG_WENT_DOWN 0x04
// and this is for objects already in cycle candidates buffer -
#define PVM_OBJECT_AH_ALLOCATOR_FLAG_IN_BUFFER 0x08




#endif // PVM_ALLOC_H
