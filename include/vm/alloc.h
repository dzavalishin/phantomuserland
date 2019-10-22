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

void pvm_memcheck(void);
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

#define PHANTOM_ARENA_FOR_INT             (1<<1) // For allocation of ints
#define PHANTOM_ARENA_FOR_STACK           (1<<2) // For allocation of stack frames
#define PHANTOM_ARENA_FOR_STATIC          (1<<3) // For allocation of classes, interfaces, code, etc
#define PHANTOM_ARENA_FOR_SMALL           (1<<4) // For allocation of < 1K

#define PHANTOM_ARENA_FOR_THREAD_INT      (1<<7) // Thread personal ints
#define PHANTOM_ARENA_FOR_THREAD_STACK    (1<<8) // Thread personal stack frames
#define PHANTOM_ARENA_FOR_THREAD_SMALL    (1<<8) // Thread personal small objects

#define PHANTOM_ARENA_FOR_1K              (1<<10) // For [1,2[K
#define PHANTOM_ARENA_FOR_2K              (1<<11) // For [2,4[K
// And so on, must be allocated on request





struct data_area_4_arena
{
    int32_t                             arena_start_marker;

    void *                              base;     //< Base address of this arena

    void *                              curr;     //< Last allocator position

    size_t                              size;     //< exact distance from my start to start of next arena object or end of memory
    size_t                              free;     //< total free mem here - UNUSED
    size_t                              largest;  //< largest free space here - UNUSED

    pvm_object_t                        owner;    //< If thread local arena - pointer to thread? No - will keep thread from being freed - UNUSED

    u_int32_t                           flags;    //< type of arena - int, permanent, small, large, etc

    // Must be recreated on OS restart
    hal_mutex_t                         mutex;
};

typedef struct data_area_4_arena persistent_arena_t;


typedef void (*arena_iterator_t)( persistent_arena_t *, void *arg );
void alloc_for_all_arenas( arena_iterator_t iter, void *arg );


extern int is_object_storage_initialized( void );
extern pvm_object_storage_t *find_root_object_storage( void );

extern persistent_arena_t * find_arena(unsigned int size, unsigned int flags, bool saturated);
// Attempt not to use - it is possible that not all arena descriptors are loaded now
//extern persistent_arena_t * find_arena_by_address(void *memaddr);

pvm_object_storage_t *alloc_eat_some(pvm_object_storage_t *op, unsigned int size);
void init_free_object_header( pvm_object_storage_t *op, unsigned int size );

void alloc_init_arenas( void * _pvm_object_space_start, size_t o_space_size );









#endif // PVM_ALLOC_H
