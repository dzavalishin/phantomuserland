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


#ifndef SYSCALL_TOOLS_H
#define SYSCALL_TOOLS_H

#include "vm/object.h"
#include "vm/stacks.h"
#include "vm/p2c.h"

// --------------------------------------------------------------------------
// Macros for syscall bodies
// --------------------------------------------------------------------------


// push on object stack
#define SYSCALL_RETURN(obj) do { pvm_ostack_push( tc->_ostack, obj ); return 1; } while(0)
#define SYSCALL_THROW(obj) do { pvm_ostack_push( tc->_ostack, obj ); return 0; } while(0)
#define SYSCALL_RETURN_NOTHING SYSCALL_RETURN( pvm_create_null_object() )

#define SYSCALL_THROW_STRING(str) SYSCALL_THROW(pvm_create_string_object( str ));

#define POP_ARG ( pvm_ostack_pop( tc->_ostack ))
#define POP_ISTACK ( pvm_istack_pop( tc->_istack ))

#define DECLARE_SIZE(class) int n_syscall_table_4_##class =	(sizeof syscall_table_4_##class) / sizeof(syscall_func_t)
//#define DECLARE_SIZE(class)

#define CHECK_PARAM_COUNT(n_param, must_have) \
    do { \
    if( n_param < must_have ) \
    	SYSCALL_THROW(pvm_create_string_object( "sys: need more parameters" )); \
    } while(0)


#define ASSERT_STRING(obj) \
    do { \
	if( !IS_PHANTOM_STRING(obj) ) \
            SYSCALL_THROW_STRING( /*"not a string arg: "*/  __func__ ); \
    } while(0)


#define ASSERT_INT(obj) \
    do { \
	if( !IS_PHANTOM_INT(obj) ) \
            SYSCALL_THROW_STRING("not an integer arg"  ); \
    } while(0)

#define DEBUG_INFO \
if( debug_print) printf("\n\n --- syscall %s at %s line %d called ---\n\n", __func__, __FILE__, __LINE__ )

// --------------------------------------------------------------------------
// Int/string parameters shortcuts
// --------------------------------------------------------------------------

#define SYS_FREE_O(o) ref_dec_o(o)

// Depends on GCC {} value extension

#define POP_INT() \
    ({ \
    struct pvm_object __ival = POP_ARG; \
    ASSERT_INT(__ival); \
    int v = pvm_get_int(__ival); \
    SYS_FREE_O(__ival); \
    v; \
    })

/* can't be used since string has to be SYS_FREE_'ed
#define POP_STRING() \
    ({ \
    struct pvm_object __sval = POP_ARG; \
    ASSERT_STRING(__sval); \
    pvm_object_da( __sval, string ); \
    })
*/


// --------------------------------------------------------------------------
// Thread sleep/wakeup
// --------------------------------------------------------------------------

#define SYSCALL_PUT_THIS_THREAD_ASLEEP() phantom_thread_put_asleep( tc )
#define SYSCALL_WAKE_THIS_THREAD_UP() phantom_thread_wake_up( tc )

#define SYSCALL_PUT_THREAD_ASLEEP(thread) phantom_thread_put_asleep( thread )
#define SYSCALL_WAKE_THREAD_UP(thread) phantom_thread_wake_up( thread )

// --------------------------------------------------------------------------
// Types and syscall table declaration macro
// --------------------------------------------------------------------------

struct pvm_object;
struct data_area_4_thread;

typedef int (*syscall_func_t)(struct pvm_object o, struct data_area_4_thread *tc );
/*
#define DECLARE_SYSTABLE(type,tab_id) \
    extern syscall_func_t syscall_table_4_##type[]; extern int n_syscall_table_4_##type; \
    void pvm_syscall_init_##tab_id() { pvm_exec_systables[tab_id] = (syscall_table_4_##type); }
*/

// --------------------------------------------------------------------------
// Default syscall implementations for lower calls
// --------------------------------------------------------------------------

int invalid_syscall(struct pvm_object o, struct data_area_4_thread *tc );
int si_void_0_construct(struct pvm_object , struct data_area_4_thread *tc );
int si_void_1_destruct(struct pvm_object , struct data_area_4_thread *tc );
int si_void_2_class(struct pvm_object this_obj, struct data_area_4_thread *tc );
int si_void_3_clone(struct pvm_object , struct data_area_4_thread *tc );
int si_void_4_equals(struct pvm_object me, struct data_area_4_thread *tc );
int si_void_5_tostring(struct pvm_object , struct data_area_4_thread *tc );
int si_void_6_toXML(struct pvm_object , struct data_area_4_thread *tc );
int si_void_7_fromXML(struct pvm_object , struct data_area_4_thread *tc );
int si_void_8_def_op_1(struct pvm_object , struct data_area_4_thread *tc );
int si_void_9_def_op_2(struct pvm_object , struct data_area_4_thread *tc );
int si_void_15_hashcode(struct pvm_object me, struct data_area_4_thread *tc );


#endif // SYSCALL_TOOLS_H
