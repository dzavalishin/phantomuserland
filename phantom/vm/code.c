/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Read bytecode from memory.
 *
 *
**/

#include "phantom_libc.h"

#include "vm/code.h"
#include "vm/exec.h"
#include "vm/object_flags.h"
#include "vm/exception.h"

#define hal_printf printf

#define int_size() 4
#define long_size() 8


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

unsigned long long
pvm_code_do_get_int64( const unsigned char *addr )
{
    unsigned long v;

    v = addr[7];
    v |= ((unsigned long long)addr[6]) << 8;
    v |= ((unsigned long long)addr[5]) << 16;
    v |= ((unsigned long long)addr[4]) << 24;
    v |= ((unsigned long long)addr[3]) << 32;
    v |= ((unsigned long long)addr[2]) << 40;
    v |= ((unsigned long long)addr[1]) << 48;
    v |= ((unsigned long long)addr[0]) << 56;

    return (long)v;
}



static void
throw_bounds( int ip, int max_IP, char *where )
{
    char errtext[200];
    snprintf(errtext, sizeof(errtext)-1, "%s: IP out of bounds (IP=%d, max=%d)", where, ip, max_IP );
    pvm_exec_panic0( errtext );
}


void pvm_code_check_bounds( struct pvm_code_handler *code, unsigned int ip, char *where )
{
    if( ip > code->IP_max )
        throw_bounds( ip, code->IP_max, where );
}

// TODO FIXME will crash if used at end of file
unsigned char pvm_code_get_byte_speculative(struct pvm_code_handler *code)
{
    pvm_code_check_bounds( code, code->IP, "get_byte" );
    return (unsigned char)code->code[code->IP];  // do not increment IP !
}

// TODO FIXME will crash if used at end of file
unsigned char pvm_code_get_byte_speculative2(struct pvm_code_handler *code)
{
    pvm_code_check_bounds( code, code->IP, "get_byte" );
    return (unsigned char)code->code[code->IP + 1];  // do not increment IP !
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

int64_t pvm_code_get_int64(struct pvm_code_handler *code)
{
    pvm_code_check_bounds( code, code->IP+long_size()-1, "get_int64" );
    unsigned long long ret = pvm_code_do_get_int64( code->code+code->IP );
    code->IP += long_size();
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
    return pvm_create_string_object_binary( (const char *)sp, len );
}


/**
 *
 * Call frame setup
 *
 **/

void pvm_call_frame_init_code(struct data_area_4_call_frame *cf, struct pvm_object code)
{
    if( !(code.data->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE) )
        pvm_exec_panic0("exec: not a code object");

    struct data_area_4_code *da = (struct data_area_4_code *)&(code.data->da);

    cf->code = da->code;
    cf->IP_max = da->code_size;
    cf->IP = 0;
    //cf->cs = code; // for gc only - work without
}














