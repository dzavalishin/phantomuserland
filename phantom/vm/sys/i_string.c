/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * .internal.string class implementation
 * 
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalClasses>
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalMethodWritingGuide>
 *
**/


#define DEBUG_MSG_PREFIX "vm.sysc.string"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <stdlib.h>

#include <phantom_libc.h>
#include <threads.h>

#include <kernel/snap_sync.h>
#include <kernel/vm.h>
#include <kernel/atomic.h>
#include <kernel/json.h>

#include <vm/syscall.h>
#include <vm/object.h>
#include <vm/object_flags.h>
#include <vm/root.h>
#include <vm/exec.h>
#include <vm/bulk.h>
#include <vm/alloc.h>
#include <vm/internal.h>
#include <vm/json.h>

#include <vm/p2c.h>

#include <vm/wrappers.h>


static int debug_print = 0;






static int si_string_3_clone( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    ASSERT_STRING(me);
    struct data_area_4_string *meda = pvm_object_da( me, string );
    SYSCALL_RETURN(pvm_create_string_object_binary( (char *)meda->data, meda->length ));
}

static int si_string_4_equals( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);
    pvm_object_t him = args[0];

    int iret = 0;
    if( !pvm_is_null(him) )
    {
        ASSERT_STRING(him);

        struct data_area_4_string *meda = pvm_object_da( me, string );
        struct data_area_4_string *himda = pvm_object_da( him, string );

        iret =
            me->_class == him->_class &&
            meda->length == himda->length &&
            0 == strncmp( (const char*)meda->data, (const char*)himda->data, meda->length )
            ;
    }
    SYS_FREE_O(him);

    // BUG - can compare just same classes? Call his .compare if he is not string?
    SYSCALL_RETURN(pvm_create_int_object( iret ) );
}

static int si_string_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    SYSCALL_RETURN(ref_inc_o(me));
}

static int si_string_8_substring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    ASSERT_STRING(me);
    struct data_area_4_string *meda = pvm_object_da( me, string );
    
    CHECK_PARAM_COUNT(2);

    int parmlen = AS_INT(args[1]);
    int index = AS_INT(args[0]);


    if( index < 0 || index >= meda->length )
        SYSCALL_THROW_STRING( "string.substring index is out of bounds" );

    int len = meda->length - index;
    if( parmlen < len ) len = parmlen;

    if( len < 0 )
        SYSCALL_THROW_STRING( "string.substring length is negative" );

    SYSCALL_RETURN(pvm_create_string_object_binary( (char *)meda->data + index, len ));
}

// TODO rename to byteat
static int si_string_9_charat( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    ASSERT_STRING(me);
    struct data_area_4_string *meda = pvm_object_da( me, string );
    
    CHECK_PARAM_COUNT(1);
    int index = AS_INT(args[0]);

    int len = meda->length;

    if(index > len-1 )
        SYSCALL_THROW_STRING( "string.charAt index is out of bounds" );

    SYSCALL_RETURN(pvm_create_int_object( meda->data[index]  ));
}


static int si_string_10_concat( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);
    pvm_object_t him = args[0];
    ASSERT_STRING(him);

    struct data_area_4_string *meda = pvm_object_da( me, string );
    struct data_area_4_string *himda = pvm_object_da( him, string );

    pvm_object_t oret = pvm_create_string_object_binary_cat(
                (char *)meda->data, meda->length,
                (char *)himda->data, himda->length );

    SYS_FREE_O(him);

    SYSCALL_RETURN( oret );
}


static int si_string_11_length( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    ASSERT_STRING(me);
    struct data_area_4_string *meda = pvm_object_da( me, string );
    
    CHECK_PARAM_COUNT(0);

    SYSCALL_RETURN(pvm_create_int_object( meda->length ));
}

static int si_string_12_find( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);
    pvm_object_t him = args[0];
    ASSERT_STRING(him);

    struct data_area_4_string *meda = pvm_object_da( me, string );
    struct data_area_4_string *himda = pvm_object_da( him, string );

    unsigned char * cret = (unsigned char *)strnstrn(
                (char *)meda->data, meda->length,
                (char *)himda->data, himda->length );

    SYS_FREE_O(him);

    int pos = -1;

    if( cret != 0 )
        pos = cret - (meda->data);

    SYSCALL_RETURN(pvm_create_int_object( pos ));
}


static int si_string_16_toint( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;    
    CHECK_PARAM_COUNT(0);

    struct data_area_4_string *meda = pvm_object_da( me, string );
    SYSCALL_RETURN(pvm_create_int_object( atoin( (char *)meda->data, meda->length ) ) );
}


static int si_string_17_tolong( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;   
    CHECK_PARAM_COUNT(0);

    struct data_area_4_string *meda = pvm_object_da( me, string );
    SYSCALL_RETURN(pvm_create_long_object( atoln( (char *)meda->data, meda->length ) ) );
}

static int si_string_18_tofloat( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;   
    CHECK_PARAM_COUNT(0);

    //struct data_area_4_string *meda = pvm_object_da( me, string );
    //SYSCALL_RETURN(pvm_create_float_object( atofn( (char *)meda->data, meda->length ) ) );
    SYSCALL_THROW_STRING("not impl tofloat");
}

static int si_string_19_todouble( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;   
    CHECK_PARAM_COUNT(0);

    //struct data_area_4_string *meda = pvm_object_da( me, string );
    //SYSCALL_RETURN(pvm_create_double_object( atodn( (char *)meda->data, meda->length ) ) );
    SYSCALL_THROW_STRING("not impl si_string_19_todouble");
}



static int si_string_20_parse_json( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;    
    CHECK_PARAM_COUNT(0);
    pvm_object_t top = pvm_json_parse_ext( pvm_get_str_data(me), pvm_get_str_len(me) );
    // TODO kill me
    //pvm_scan_print_subtree( top, 20 );
    SYSCALL_RETURN( top );
}

extern int si_void_14_to_immutable( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args );

syscall_func_t  syscall_table_4_string[21] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_string_3_clone,
    &si_string_4_equals,            &si_string_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_string_8_substring,         &si_string_9_charat,
    &si_string_10_concat,           &si_string_11_length,
    &si_string_12_find,             &invalid_syscall,
    &si_void_14_to_immutable,       &si_void_15_hashcode,
    // 16
    &si_string_16_toint,            &si_string_17_tolong,
    &si_string_18_tofloat,          &si_string_19_todouble,
    &si_string_20_parse_json
};
DECLARE_SIZE(string);


