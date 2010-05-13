/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * JIT codegen - machine independent part
 *
 *
 *
**/

#include "jit.h"

int jit_init( jit_out_t *j )
{
    j->nextLabel = 0;
}


// Put byte to output code buffer
void jit_code_8( jit_out_t *j, u_int8_t code )
{
}

// Put 32 bit int to output code buffer
void jit_code_32( jit_out_t *j, u_int32_t code )
{
}




int jit_get_label( jit_out_t *j )
{
    return j->nextLabel++;
}


void jit_mark_label( j, jl )
{
    // Set label targer to current binary instr ptr

    // Also make sure this ip is in crssref ip table
}

