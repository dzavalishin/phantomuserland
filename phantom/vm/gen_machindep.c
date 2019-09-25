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
    /*
    j->nextLabel = 0;
    j->buf = 0;
    j->bufp = 0;
    j->bufsize = 0;
    */

    return 0;
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
    //_SF_( JIT_F_THREAD_SLEEP_WORKER,    phantom_thread_sleep_worker );
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


