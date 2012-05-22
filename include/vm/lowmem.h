/**
 *
 * Phantom OS
 *
 * Copyright (C) 2012-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * Bottom of persistent memory, arenas. NOT USED YET, IN WORK!
 *
 *
**/


#ifndef LOWMEM_H
#define LOWMEM_H

#include <phantom_types.h>


// This structure is kept in lowest address of persistent memory pool.
// Used to keep address of root object and memory arena control info.

struct vm_lowmem
{
    u_int64_t		root_object_ptr;
    u_int64_t		memsize;
    u_int64_t		first_arena_ptr;
    u_int64_t		first_unused_ptr;	// pointer to first unused byte (place to alloc next arena)
};

#define VM_ARENA_FLAG_STATIC		(1<<0)		// Objects supposed to be not deleted at all (not enforced, though)
#define VM_ARENA_FLAG_THREAD		(1<<1)		// Per-thread allocation of small objects

#define VM_ARENA_FLAG_ACTIVE		(1<<3)		// Can be used for allocation

#define VM_ARENA_FLAG_LARGE_1K		(1<<10)		// Allocation of large objects
#define VM_ARENA_FLAG_LARGE_8K		(1<<11)		// Allocation of large objects
#define VM_ARENA_FLAG_LARGE_64K		(1<<12)		// Allocation of large objects
#define VM_ARENA_FLAG_LARGE_512K	(1<<13)		// Allocation of large objects

#define VM_ARENA_FLAG_LARGE_8M		(1<<14)		// Allocation of large objects
#define VM_ARENA_FLAG_LARGE_128M	(1<<15)		// Allocation of large objects

#define VM_ARENA_FLAG_LARGE_SINGLE	(1<<20)		// Allocation of very large object - one per arena

typedef struct vm_arena
{
    u_int64_t		size;
    u_int64_t		flags; // arena type
} vm_arena_t;


errno_t		vm_allocate_arena( vm_arena_t **a, int flags, native_t size ); // size -1 for automatic
errno_t		vm_deactivate_arena( vm_arena_t *a );
errno_t		vm_concatenate_dead_arenas();



#endif // LOWMEM_H
