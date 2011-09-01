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

#include <phantom_assert.h>
#include <kernel/page.h>
#include <phantom_types.h>

#include <malloc.h>

#include "vm/root.h"
#include "vm/internal_da.h"
#include "vm/internal.h"
#include "vm/object_flags.h"
#include "vm/exception.h"
#include "vm/alloc.h"

#include "vm/exec.h"
#include "vm/code.h"

#include "vm/stacks.h"
#include "vm/syscall.h"

#include "ids/opcode_ids.h"

#include <kernel/snap_sync.h>
#include <kernel/page.h>

#include "jit.h"

int jit_init_unit( jit_out_t *j )
{
    j->nextLabel = 0;
    j->buf = 0;
    j->bufp = 0;
    j->bufsize = 0;

    return 0;
}


void jit_checkbuf( jit_out_t *j, int size )
{
    if( j->buf == 0 )
    {
        j->bufsize = PAGE_SIZE;

        // TODO calc it! PAGE_ALIGN(size)?
        while( j->bufsize < size )
            j->bufsize += PAGE_SIZE;

        j->buf = calloc( 1, j->bufsize );
        j->bufp = j->buf;

        assert(j->buf);

        return;
    }

    int used = j->bufp - j->buf;

    if( j->bufsize - used > size )
        return;

    //int shift = bufp - buf;

    int oldsize = j->bufsize;

    j->bufsize += PAGE_SIZE;
    char *newb = calloc( 1, j->bufsize );
    memmove( newb, j->buf, oldsize );
    free( j->buf );

    j->buf = newb;

    j->bufp = j->buf+used;
}


// Put byte to output code buffer
void jit_code_8( jit_out_t *j, u_int8_t code )
{
    jit_checkbuf( j, 1 );
    *j->bufp++ = code;
}

// Put 32 bit int to output code buffer
void jit_code_32( jit_out_t *j, u_int32_t code )
{
    jit_checkbuf( j, 4 );
    *((u_int32_t*)j->bufp) = code;
    j->bufp += sizeof(u_int32_t);
}


// Put binary code part to output code buffer
void copy_jit_code( jit_out_t *j, void *code, size_t size )
//void copy_jit_code( jit_out_t *j, char * code, int size )
{
    jit_checkbuf( j, size );
    memmove( j->bufp, code, size );
    j->bufp += size;
}



int jit_get_label( jit_out_t *j )
{
    return j->nextLabel++;
}


void jit_mark_label( jit_out_t *j, int jl )
{
    // Set label targer to current binary instr ptr

    // Also make sure this ip is in crssref ip table
}


// --------------------------------------------------------------------------
// Func tab
// --------------------------------------------------------------------------

typedef void (*ft_entry_t)(void);

//void (*jit_kernel_func_table)[JIT_FUNC_TABLE_SIZE];
ft_entry_t jit_kernel_func_table[JIT_FUNC_TABLE_SIZE];

#define _SF_(pos, func) do { jit_kernel_func_table[pos] = (void *) func; } while(0)
void jit_kernel_func_table_init()
{
    _SF_( JIT_F_LOAD_F_ACC, 		pvm_exec_load_fast_acc );
    _SF_( JIT_F_THREAD_SLEEP_WORKER,    phantom_thread_sleep_worker );
    _SF_( JIT_F_WAIT_SNAP, 		phantom_thread_wait_4_snap );
    _SF_( JIT_F_CODE_GET_BYTE, 		pvm_code_get_byte );
    _SF_( JIT_F_CREATE_OBJ, 		pvm_create_object );
    _SF_( JIT_F_COPY_OBJ,               pvm_copy_object );
    _SF_( JIT_F_CREATE_INT_OBJ,         pvm_create_int_object );

};


// --------------------------------------------------------------------------
// Setup JIT engine
// --------------------------------------------------------------------------


void jit_init(void)
{
    jit_kernel_func_table_init();

    // TODO get persistent object used to store func table,

    // Copy actual table there

    // TODO mem map JITted code as executable
}


