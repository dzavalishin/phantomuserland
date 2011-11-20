/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Object allocator header
 *
 *
**/

#ifndef PVM_ALLOC_H
#define PVM_ALLOC_H

#include <hal.h>
#include <vm/object.h>





// allocator

pvm_object_storage_t * pvm_object_alloc( unsigned int data_area_size, unsigned int flags, bool saturated );
//void pvm_object_delete( pvm_object_storage_t * );


void pvm_alloc_init( void * _pvm_object_space_start, unsigned int size );
void pvm_alloc_threaded_init(void);

void pvm_alloc_clear_mem(void);

pvm_object_storage_t *get_root_object_storage(void);

int pvm_memcheck(void);
bool pvm_object_is_allocated_light(pvm_object_storage_t *p);
bool pvm_object_is_allocated(pvm_object_storage_t *p);
void pvm_object_is_allocated_assert(pvm_object_storage_t *p);
void print_object_flags(pvm_object_storage_t *p);
void debug_catch_object(const char *msg, pvm_object_storage_t *p);


// gc

void run_gc(void);

// Make sure this object won't be deleted with refcount dec
// used on sys global objects
void ref_saturate_o(pvm_object_t o);

void          ref_dec_o(pvm_object_t o);
pvm_object_t  ref_inc_o(pvm_object_t o);

void ref_dec_p(pvm_object_storage_t *p);
void ref_inc_p(pvm_object_storage_t *p);


void do_ref_dec_p(pvm_object_storage_t *p); // for deferred refdec



// ------------------------------------------------------------
// shared between alloc.c and gc.c

// Gigant lock for now. TODO
extern hal_mutex_t  *vm_alloc_mutex;


void * get_pvm_object_space_start(void);
void * get_pvm_object_space_end(void);

void refzero_process_children( pvm_object_storage_t *o );
void ref_saturate_p(pvm_object_storage_t *p);

// called by refcount code - collapse free objects and attempt to 
// unmap 'em
void pvm_collapse_free(pvm_object_storage_t *op); 




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


// ------------------------------------------------------------
// Persistent arenas machinery - in progress
// ------------------------------------------------------------

#include <kernel/mutex.h>

#define PVM_ARENA_START_MARKER 0xAAAA77FE

// Contains nothing at all, unallocated arena space
#define PVM_ARENA_FLAG_EMPTY            (1<<0)
// Contains objects, not arenas
#define PVM_ARENA_FLAG_LEAF             (1<<1)
// This is a per-thread arena - how do we find thread after restoring snap?
#define PVM_ARENA_FLAG_THREAD           (1<<2)

#define PVM_ARENA_FLAG_INT              (1<<8)
#define PVM_ARENA_FLAG_FAST             (1<<9)
#define PVM_ARENA_FLAG_SATURATED        (1<<10)

// Object size < 1K
#define PVM_ARENA_FLAG_1K               (1<<16)
// Object size < 16K
#define PVM_ARENA_FLAG_16K              (1<<17)
#define PVM_ARENA_FLAG_64K              (1<<18)
#define PVM_ARENA_FLAG_512K             (1<<19)


struct persistent_arena
{
    int32_t             arena_start_marker;

    // from beginning of this struct to arena end
    int64_t             arena_full_size; 

    int32_t             arena_flags;

    // pointer to owning thread (data part) - used on startup only, not counted.
    // must be cleaned by GC if thread is collected.
    int64_t             arena_thread_ptr;

    // Must be recreated on OS restart
    hal_mutex_t         arena_mutex;
};

typedef struct persistent_arena persistent_arena_t;


















#endif // PVM_ALLOC_H
