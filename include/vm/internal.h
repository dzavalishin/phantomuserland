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

#include "vm/root.h"
#include "vm/syscall.h"


typedef void (*init_func_t)( struct pvm_object_storage * os );
typedef void (*gc_iterator_call_t)( struct pvm_object o, void *arg );
typedef void (*gc_iterator_func_t)( gc_iterator_call_t func, struct pvm_object_storage * os, void *arg );

typedef void (*gc_finalizer_func_t)( struct pvm_object_storage * os );


struct internal_class
{
    const char *                name;
    int                         root_index; 		// index into the root object, where class is stored
    syscall_func_t *            syscalls_table;         // syscalls implementations
    init_func_t                 init;                   // constructor
    gc_iterator_func_t          iter;                   // Call func for all the object pointer contained in this internal object
    gc_finalizer_func_t         finalizer;
    int                         da_size;
    int                         flags;
    struct pvm_object           class_object;           // inited on start, used for lookup
};

extern int pvm_n_internal_classes;
extern struct internal_class pvm_internal_classes[];


int     pvm_iclass_by_root_index( int index );
int     pvm_iclass_by_class( struct pvm_object_storage *cs );
int     pvm_iclass_by_name( const char *name );         // For internal class lookup

struct pvm_object pvm_lookup_internal_class(struct pvm_object name);


// Init (fast constructor) functions

#define DEF_I(cn) \
    extern syscall_func_t	syscall_table_4_##cn[]; \
    extern void pvm_internal_init_##cn( struct pvm_object_storage * os ); \
    extern void pvm_gc_iter_##cn( gc_iterator_call_t func, struct pvm_object_storage * os, void *arg ); \
    extern void pvm_gc_finalizer_##cn( struct pvm_object_storage * os );

DEF_I(void);
DEF_I(class)
DEF_I(interface)
DEF_I(code)
DEF_I(int)
DEF_I(string)
DEF_I(array)
DEF_I(page)
DEF_I(thread)
DEF_I(call_frame)
DEF_I(istack)
DEF_I(ostack)
DEF_I(estack)
DEF_I(boot)
DEF_I(tty)
DEF_I(mutex)
DEF_I(binary)
DEF_I(bitmap)
DEF_I(world)
DEF_I(closure)
DEF_I(weakref)
DEF_I(window)

#undef DEF_I


