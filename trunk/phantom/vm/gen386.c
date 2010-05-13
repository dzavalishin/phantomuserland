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

void jit_jz( jit_out_t *, int jlabel )
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

void jit_iconst( jit_out_t *j, int const) // mov 0, %ax
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


jit_refinc( jit_out_t *j, int reg )
{
}

jit_refdec( jit_out_t *j, int reg )
{
}


// --------------------------------------------------------------------------
// Summon
// --------------------------------------------------------------------------

void jit_get_null( jit_out_t *j ) // AX = 0 DX = 0
{
    // mov $0, %ax
    // mov $0, %dx
}

void jit_get_thread( jit_out_t *j )// AX, DX = thread ptr
{
}


void jit_get_this( jit_out_t *j ) // AX, DX = this ptr
{
    // mov %bx, %ax
}

void jit_get_class_class( jit_out_t *j ) // AX, DX = ptr
{
}

void jit_get_iface_class( jit_out_t *j ) // AX, DX = ptr
{
}

void jit_get_code_class( jit_out_t *j ) // AX, DX = ptr
{
}

void jit_get_int_class( jit_out_t *j ) // AX, DX = ptr
{
}

void jit_get_strting_class( jit_out_t *j ) // AX, DX = ptr
{
}

void jit_get_array_class( jit_out_t *j ) // AX, DX = ptr
{
}

