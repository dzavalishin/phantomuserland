/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Network syscalls?
 * 
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalClasses>
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalMethodWritingGuide>
 *
**/


#define DEBUG_MSG_PREFIX "vm.sysc.net"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 1
#define debug_level_info 0


#include <phantom_libc.h>
#include <vm/syscall_net.h>
#include <vm/alloc.h>
#include <kernel/snap_sync.h>
#include <kernel/net.h>
#include <kernel/json.h>

#include <errno.h>


static int debug_print = 0;





static int si_tcp_tostring_5( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    DEBUG_INFO;
    SYSCALL_RETURN( pvm_create_string_object( "tcp socket" ));
}


static int si_tcp_connect_16( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    //struct data_area_4_tcp      *da = pvm_data_area( me, tcp );

    DEBUG_INFO;
    

    CHECK_PARAM_COUNT(2);

    //SYSCALL_PUT_THIS_THREAD_ASLEEP()

    SYSCALL_THROW_STRING( "not implemented" );
}

static int si_tcp_disconnect_17( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    //struct data_area_4_tcp      *da = pvm_data_area( me, tcp );

    DEBUG_INFO;
    

    CHECK_PARAM_COUNT(2);


    SYSCALL_THROW_STRING( "not implemented" );
}





static int si_tcp_send_19( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    //struct data_area_4_tcp      *da = pvm_data_area( me, tcp );

    DEBUG_INFO;
    

    CHECK_PARAM_COUNT(2);


    SYSCALL_THROW_STRING( "not implemented" );
}






static int si_tcp_recv_21( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    //struct data_area_4_tcp      *da = pvm_data_area( me, tcp );

    DEBUG_INFO;

    CHECK_PARAM_COUNT(2);

    SYSCALL_THROW_STRING( "not implemented" );
}







static int si_tcp_accept_23( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    //struct data_area_4_tcp      *da = pvm_data_area( me, tcp );

    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(2);

    SYSCALL_THROW_STRING( "not implemented" );
}





static int si_tcp_curl_24( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    //struct data_area_4_tcp      *da = pvm_data_area( me, tcp );

    DEBUG_INFO;    
    CHECK_PARAM_COUNT(2);

    pvm_object_t url = args[0];
    pvm_object_t hdr = args[1];

    size_t url_len = pvm_get_str_len( url );
    size_t hdr_len = pvm_get_str_len( hdr );

    if( url_len > 1024 ) SYSCALL_THROW_STRING( "URL too long" );
    if( hdr_len > 1024 ) SYSCALL_THROW_STRING( "HTTP headers too long" );

    char surl[url_len+1]; strlcpy( surl, pvm_get_str_data(url), url_len+1 );
    char shdr[hdr_len+1]; strlcpy( shdr, pvm_get_str_data(hdr), hdr_len+1 );

    const int bs = 100*1024;
    char *buf = malloc(bs);
    if( 0 == buf )
    {
        SHOW_ERROR( 1, "out of mem in curl() %d bytes", bs );
        // TODO enable me and catch in userland code
        //SYSCALL_THROW_STRING( "not implemented" );
        SYSCALL_RETURN_STRING("");
    }

    buf[0] = 0;

    SHOW_FLOW( 10, "curl %s (hdr '%s')", surl, shdr );

    vm_unlock_persistent_memory();
    errno_t rc = net_curl( surl, buf, bs, shdr );
    vm_lock_persistent_memory();

    SHOW_FLOW( 10, "curl ret '%s'", buf );

    // TODO need real http protocol request impl
    if( rc && (rc != -ETIMEDOUT) )
    {
        SHOW_ERROR( 1, "curl fail %d", rc );
        // TODO enable me and catch in userland code
        //SYSCALL_THROW_STRING( "not implemented" );
        SYSCALL_RETURN_STRING("");
    }

    const char *content = http_skip_header( buf );

    SHOW_FLOW( 1, "curl content '%s' ", content );

    pvm_object_t oret = pvm_create_string_object(content);
    free(buf);

    SYSCALL_RETURN(oret); // TODO need stringBuilder
}









syscall_func_t  syscall_table_4_tcp[25] =
{
    &si_void_0_construct,               &si_void_1_destruct,
    &si_void_2_class,                   &si_void_3_clone,
    &si_void_4_equals,                  &si_tcp_tostring_5,
    &si_void_6_toXML,                   &si_void_7_fromXML,
    // 8
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,                   &si_void_15_hashcode,
    // 16
    &si_tcp_connect_16,                 &si_tcp_disconnect_17,
    &invalid_syscall,                   &si_tcp_send_19,
    &invalid_syscall,                   &si_tcp_recv_21,
    &invalid_syscall,                   &si_tcp_accept_23,
    // 24
    &si_tcp_curl_24,

};
DECLARE_SIZE(tcp);








