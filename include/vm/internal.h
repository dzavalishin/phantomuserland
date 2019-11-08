/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal (native) classes definitions.
 *
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalClasses>
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalMethodWritingGuide>
 * 
**/

#ifndef VM_INTERNAL_H
#define VM_INTERNAL_H

#include "vm/root.h"
#include "vm/syscall.h"


typedef void (*init_func_t)( pvm_object_t  os );
typedef void (*gc_iterator_call_t)( pvm_object_t o, void *arg );
typedef void (*gc_iterator_func_t)( gc_iterator_call_t func, pvm_object_t  os, void *arg );

typedef void (*gc_finalizer_func_t)( pvm_object_t  os );

typedef void (*o_restart_func_t)( pvm_object_t o );


struct internal_class
{
    const char *                name;
    int                         root_index; 		// index into the root object, where class is stored
    syscall_func_t *            syscalls_table;         // syscalls implementations
    int                         *syscalls_table_size_ptr;    // n of syscalls
    init_func_t                 init;                   // constructor
    gc_iterator_func_t          iter;                   // Call func for all the object pointer contained in this internal object
    gc_finalizer_func_t         finalizer;
    o_restart_func_t            restart;                // Called on OS restart if object put itself to restart list, see root.c
    int                         da_size;
    int                         flags;
    pvm_object_t                class_object;           // inited on start, used for lookup
};

extern int pvm_n_internal_classes;
extern struct internal_class pvm_internal_classes[];


int     pvm_iclass_by_root_index( int index );
int     pvm_iclass_by_class( pvm_object_t cs );
int     pvm_iclass_by_name( const char *name );         // For internal class lookup

pvm_object_t pvm_lookup_internal_class(pvm_object_t name);


// Init (fast constructor) functions

#define DEF_I(cn) \
    extern syscall_func_t	syscall_table_4_##cn[]; \
	extern int n_syscall_table_4_##cn; \
    extern void pvm_internal_init_##cn( pvm_object_t  os ); \
    extern void pvm_gc_iter_##cn( gc_iterator_call_t func, pvm_object_t  os, void *arg ); \
    extern void pvm_gc_finalizer_##cn( pvm_object_t  os ); \
    extern void pvm_restart_##cn( pvm_object_t o );

//#define DEF_SIZE(cn)    const int n_syscall_table_4_##cn = (sizeof syscall_table_4_##cn) / sizeof(syscall_func_t); 
#define DECLARE_SIZE(__cn) int n_syscall_table_4_##__cn __attribute__((used)) =	(sizeof syscall_table_4_##__cn) / sizeof(syscall_func_t)

DEF_I(void);
DEF_I(class)
DEF_I(interface)
DEF_I(code)
DEF_I(int)
DEF_I(long)
DEF_I(float)
DEF_I(double)
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
DEF_I(cond)
DEF_I(sema)
DEF_I(binary)
DEF_I(bitmap)
DEF_I(world)
DEF_I(closure)
DEF_I(weakref)
DEF_I(window)
DEF_I(directory)
DEF_I(connection)
//DEF_I(hash)

DEF_I(tcp)
DEF_I(udp)

DEF_I(net)
DEF_I(http)

DEF_I(time)
DEF_I(stat)

DEF_I(io)
DEF_I(port)
DEF_I(ui_control)
DEF_I(ui_font)

DEF_I(stringbuilder)

DEF_I(crypt)

#undef DEF_I


#endif // VM_INTERNAL_H
