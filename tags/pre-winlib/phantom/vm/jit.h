/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * JIT header
 *
 *
 *
**/

#ifndef JIT_H
#define JIT_H

#include <sys/types.h>
#include "vm/internal_da.h"

void jit_init(void);

//! Compile JIT code for class
//void jit_compile_class(struct data_area_4_class *da);

//! Compile JIT code for method
errno_t jit_compile_method(struct pvm_code_handler *code);

/*!
 *
 * struct data_area_4_class *da - class
 * void *b_code                 - method bytecode
 * int bc_size                  - size of bytecode
 * pvm_object_t out             - output (binary) variable to put binary code to
 *
 */
//void jit_compile_method( struct data_area_4_class *da, void *b_code, int bc_size, pvm_object_t out );

// --------------------------------------------------------------------------
// Enter/leave JIT code
// --------------------------------------------------------------------------


// JIT code runs with:
// BX = thread da
// ?? = this



// =========================================================================
//                                        **
//                  *                      *
// ** ***   ****   *****    *****          * ***    ****   ** ***   ****
//  **  *  *    *   *           *          **   *  ******   **  *  ******
//  *   *  *    *   *   *  ******          *    *  *        *   *  *
// *** ***  ****    ****   **** **        ******    *****  *** ***  *****
//
// Don't you EVER remove or change there funcs. Each instance of OS keeps
// these ordinals in JITted code.
//
// Or we have to find way to force OS instance to reJIT on some serious
// change. Any kernel change, for example?
//
// =========================================================================



enum jit_callable
{
    JIT_F_LOAD_F_ACC, 			// pvm_exec_load_fast_acc
    JIT_F_THREAD_SLEEP_WORKER,          // phantom_thread_sleep_worker
    JIT_F_WAIT_SNAP, 			// phantom_thread_wait_4_snap
    JIT_F_CODE_GET_BYTE, 		// pvm_code_get_byte
    JIT_F_CREATE_OBJ, 			// pvm_create_object
    JIT_F_COPY_OBJ,                     // pvm_copy_object
    JIT_F_CREATE_INT_OBJ,               // pvm_create_int_object

    // LAST!
    JIT_FUNC_TABLE_SIZE
};

//void jit_kernel_func_table_init(void);



struct jit_out
{
    //! Next label id
    //int 	nextLabel;

    // Code buffer
    //char *	buf;
    //char *	bufp; // Current put pos
    //int   	bufsize;

    // map of interpreted IP to asm instr offset

    // map of callable kernel funcs - id to func
};


typedef struct jit_out jit_out_t;


struct jit_value
{
};

typedef struct jit_value jit_value_t;



// --------------------------------------------------------------------------
// Misc
// --------------------------------------------------------------------------

// Store curr IP as possible jump target
void  jit_mark_possible_label(jit_out_t *j);


jit_value_t jit_o2int( jit_out_t *j, jit_value_t v );
jit_value_t jit_gen_isnull( jit_out_t *j, jit_value_t ); // dst = (dst == 0)
void jit_check_snap_request( jit_out_t *j );

// TODO const list is just an array? easy to release all
int jit_alloc_const( jit_out_t *j, pvm_object_t s );
jit_value_t jit_get_const( jit_out_t *j, int id );


void jit_push_arg( jit_out_t *j, jit_value_t v );
jit_value_t jit_gen_call( jit_out_t *, enum jit_callable );

jit_value_t jit_call_method( jit_out_t *j, int n_method, int n_param, int flags );


void jit_ret( jit_out_t *j );
void jit_throw( jit_out_t *j, jit_value_t v );

jit_value_t jit_sys( jit_out_t *, int nsys ); // TODO fall asleep there


void jit_jump( jit_out_t *j, int newip );
void jit_jnz( jit_out_t *j, int newip, jit_value_t v );
void jit_jz( jit_out_t *j, int newip, jit_value_t v );

void jit_decrement( jit_out_t *j, jit_value_t v );




// --------------------------------------------------------------------------
// I Stack
// --------------------------------------------------------------------------


jit_value_t jit_is_pop( jit_out_t *j );
void        jit_is_push( jit_out_t *j, jit_value_t );
jit_value_t jit_is_top( jit_out_t *j );
jit_value_t jit_iconst( jit_out_t *j, int const);

jit_value_t jit_is_load( jit_out_t *j, int slot );
jit_value_t jit_is_pull( jit_out_t *j, int slot );
jit_value_t jit_is_absget( jit_out_t *j, int slot );

jit_value_t jit_is_save( jit_out_t *j, int slot, jit_value_t  );
jit_value_t jit_is_absset( jit_out_t *j, int slot, jit_value_t );


// --------------------------------------------------------------------------
// O Stack
// --------------------------------------------------------------------------

void        jit_os_push( jit_out_t *j, jit_value_t );
jit_value_t jit_os_pop( jit_out_t *j );
jit_value_t jit_os_top( jit_out_t *j );

jit_value_t jit_os_load( jit_out_t *j, int slot );
jit_value_t jit_os_pull( jit_out_t *j, int slot );
jit_value_t jit_os_absget( jit_out_t *j, int slot );

jit_value_t jit_os_save( jit_out_t *j, int slot, jit_value_t  );
jit_value_t jit_os_absset( jit_out_t *j, int slot, jit_value_t );


// --------------------------------------------------------------------------
// E Stack
// --------------------------------------------------------------------------

void jit_es_push( jit_out_t *j, jit_value_t type, int addr );
jit_value_t  jit_es_pop( jit_out_t *j );


// --------------------------------------------------------------------------
// Ref inc/dec
// --------------------------------------------------------------------------


jit_value_t jit_refinc( jit_out_t *j, jit_value_t );
void jit_refdec( jit_out_t *j, jit_value_t );


// --------------------------------------------------------------------------
// Summon
// --------------------------------------------------------------------------

jit_value_t jit_get_null( jit_out_t *j );
jit_value_t jit_get_thread( jit_out_t *j );
jit_value_t jit_get_this( jit_out_t *j ); 
jit_value_t jit_get_class_class( jit_out_t *j );
jit_value_t jit_get_iface_class( jit_out_t *j );
jit_value_t jit_get_code_class( jit_out_t *j );
jit_value_t jit_get_int_class( jit_out_t *j );
jit_value_t jit_get_string_class( jit_out_t *j );
jit_value_t jit_get_array_class( jit_out_t *j );


// --------------------------------------------------------------------------
// ops
// --------------------------------------------------------------------------



jit_value_t jit_gen_binor( struct jit_out *j, jit_value_t a, jit_value_t b );
jit_value_t jit_gen_binand( struct jit_out *j, jit_value_t a, jit_value_t b );
jit_value_t jit_gen_binxor( struct jit_out *j, jit_value_t a, jit_value_t b );
jit_value_t jit_gen_logor( struct jit_out *j, jit_value_t a, jit_value_t b );
jit_value_t jit_gen_logand( struct jit_out *j, jit_value_t a, jit_value_t b );
jit_value_t jit_gen_logxor( struct jit_out *j, jit_value_t a, jit_value_t b );
jit_value_t jit_gen_ige( struct jit_out *j, jit_value_t a, jit_value_t b );
jit_value_t jit_gen_ile( struct jit_out *j, jit_value_t a, jit_value_t b );
jit_value_t jit_gen_igt( struct jit_out *j, jit_value_t a, jit_value_t b );
jit_value_t jit_gen_ilt( struct jit_out *j, jit_value_t a, jit_value_t b );
jit_value_t jit_gen_add( struct jit_out *j, jit_value_t a, jit_value_t b );
jit_value_t jit_gen_sub( struct jit_out *j, jit_value_t a, jit_value_t b );
jit_value_t jit_gen_mul( struct jit_out *j, jit_value_t a, jit_value_t b );
jit_value_t jit_gen_div( struct jit_out *j, jit_value_t a, jit_value_t b );

jit_value_t jit_gen_cmp( struct jit_out *j, jit_value_t a, jit_value_t b );
//jit_value_t jit_gen_neg( struct jit_out *j, jit_value_t a );

jit_value_t jit_gen_binnot( struct jit_out *j, jit_value_t a );
jit_value_t jit_gen_lognot( struct jit_out *j, jit_value_t a );

jit_value_t jit_is_null( struct jit_out *j, jit_value_t a );



#endif // JIT_H


