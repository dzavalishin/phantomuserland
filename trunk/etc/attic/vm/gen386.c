/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * JIT codegen for ia32 arch.
 *
 *
 *
**/

#include "jit.h"
#include <phantom_assert.h>


#define COPYCODE(name)                          \
    extern char jit_proto_##name[]; 		\
    extern char jit_proto_##name##_end[];      	\
    copy_jit_code( j, jit_proto_##name, jit_proto_##name##_end - jit_proto_##name );


// --------------------------------------------------------------------------
// Direct instructions
// --------------------------------------------------------------------------

void jit_gen_mov( jit_out_t *j, int src, int dst ) // src reg -> dst reg
{
}

void jit_gen_cmp( jit_out_t *j, int src, int dst ) // dst = (dst == src)
{
}

void jit_gen_neg( jit_out_t *j, int dst ) // dst = !dst
{
}

void jit_jz( jit_out_t *j, int jlabel )
{
    // get dest ip from jlabel
}


void jit_gen_push( jit_out_t *j, int src ) // Push reg
{
}

void jit_gen_call( jit_out_t *j, enum jit_callable funcId )
{
}




// --------------------------------------------------------------------------
// Misc
// --------------------------------------------------------------------------




void jit_o2int( jit_out_t *j )
{
    // on entry AX -> object

    // TODO check obj 4 null, throw!

    // set flags on ax



}


void jit_gen_isnull( jit_out_t *j, int dst ) // dst = (dst == 0)
{
    // Must set flags? Suppose so.
    switch(dst)
    {
    case JIT_R_AX:    { COPYCODE(isnull_ax);      break; }
    default:
        panic("isnull !ax?");
    }
}





void jit_check_snap_request( jit_out_t *j )
{
}


// --------------------------------------------------------------------------
// I Stack
// --------------------------------------------------------------------------


void jit_is_pop( jit_out_t *j )// pop AX
{
}


void jit_is_push( jit_out_t *j )// push AX
{
}

void jit_is_top( jit_out_t *j )
{
}

void jit_iconst( jit_out_t *j, int constVal ) // mov 0, %ax
{
}


// --------------------------------------------------------------------------
// O Stack
// --------------------------------------------------------------------------



void jit_os_push( jit_out_t *j )
{
}


void jit_os_pop( jit_out_t *j )
{
}


void jit_os_top( jit_out_t *j )
{
}

// --------------------------------------------------------------------------
// Ref inc/dec
// --------------------------------------------------------------------------


void jit_refinc( jit_out_t *j, int reg )
{
}

void jit_refdec( jit_out_t *j, int reg )
{
}


// --------------------------------------------------------------------------
// Summon
// --------------------------------------------------------------------------

void jit_get_null( jit_out_t *j ) // AX = 0 DX = 0
{
    COPYCODE(get_null);
}

void jit_get_thread( jit_out_t *j )// AX, DX = thread ptr
{
}


void jit_get_this( jit_out_t *j ) // AX, DX = this ptr
{
    COPYCODE(get_this);
}

void jit_get_class_class( jit_out_t *j ) // AX, DX = ptr
{
    COPYCODE(get_class_class);
}

void jit_get_iface_class( jit_out_t *j ) // AX, DX = ptr
{
    COPYCODE(get_iface_class);
}

void jit_get_code_class( jit_out_t *j ) // AX, DX = ptr
{
    COPYCODE(get_code_class);
}

void jit_get_int_class( jit_out_t *j ) // AX, DX = ptr
{
    COPYCODE(get_int_class);

}

void jit_get_string_class( jit_out_t *j ) // AX, DX = ptr
{
    COPYCODE(get_string_class);
}

void jit_get_array_class( jit_out_t *j ) // AX, DX = ptr
{
    COPYCODE(get_array_class);
}

