/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 *
 *
 **/

#include "phantom_libc.h"

//#include "vm/internal_da.h"
//#include "vm/exec.h"
#include "vm/code.h"
#include "vm/object_flags.h"
#include "vm/exception.h"

#define hal_printf printf

#define int_size() 4


int
pvm_code_do_get_int( const unsigned char *addr )
{
    unsigned int v;

    v = addr[3];
    v |= ((unsigned int)addr[2]) << 8;
    v |= ((unsigned int)addr[1]) << 16;
    v |= ((unsigned int)addr[0]) << 24;
    return (int)v;

}

static void
throw_bounds( int ip, int max_IP, char *where )
{
    static char errtext[200]; // BUG!!
    snprintf(errtext, 199, "%s: IP out of bounds (IP=%d, max=%d)", where, ip, max_IP );
    pvm_exec_throw( errtext );
}


void pvm_code_check_bounds( struct pvm_code_handler *code, unsigned int ip, char *where )
{
    if( ip > code->IP_max )
        throw_bounds( ip, code->IP_max, where );
}

unsigned char pvm_code_get_byte_speculative(struct pvm_code_handler *code)
{
    pvm_code_check_bounds( code, code->IP, "get_byte" );
    return (unsigned char)code->code[code->IP];  // do not increment IP !
}


unsigned char pvm_code_get_byte(struct pvm_code_handler *code)
{
    pvm_code_check_bounds( code, code->IP, "get_byte" );
    return (unsigned char)code->code[code->IP++];
}

int pvm_code_get_int32(struct pvm_code_handler *code)
{
    pvm_code_check_bounds( code, code->IP+int_size()-1, "get_int32" );
    int ret = pvm_code_do_get_int( code->code+code->IP );
    code->IP += int_size();
    return ret;
}

unsigned int pvm_code_get_rel_IP_as_abs(struct pvm_code_handler *code)
{
    int here = code->IP;
    // (int) to make it signed to get bidirectional displacement
    return here + (int)pvm_code_get_int32(code);
}

struct pvm_object pvm_code_get_string(struct pvm_code_handler *code)
{
    int len = pvm_code_get_int32(code);
    const unsigned char *sp = code->code+code->IP;
    code->IP += len;
    pvm_code_check_bounds( code, code->IP-1, "get_string" );
    // after we checked there is a real data accessible we can
    // create string object
    return pvm_create_string_object_binary( sp, len );
}


/**
 *
 * Call frame setup
 *
 **/

void pvm_call_frame_init_code(struct data_area_4_call_frame *cf, struct pvm_object code)
{
    // TODO: these asserts must just raise an exception in Phantom code, nothing more. ??
    if( !(code.data->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE) )
        pvm_exec_throw("exec: not a code object");

    struct data_area_4_code *da = (struct data_area_4_code *)&(code.data->da);

    cf->code = da->code;
    cf->IP_max = da->code_size;
    cf->IP = 0;
    //cf->cs = code; // for gc only - work without
}














