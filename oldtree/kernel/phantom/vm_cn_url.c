/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * Connection to HTTP in curl style - connect, get.
 *
**/

#define DEBUG_MSG_PREFIX "vmcn.url"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <vm/connect.h>
#include <vm/alloc.h>
#include <vm/p2c.h>
#include <vm/syscall.h>


#include <kernel/net/udp.h>



errno_t cn_url_do_operation( int op_no, struct data_area_4_connection *c, struct data_area_4_thread *t, pvm_object_t o )
{
    (void) t;
    (void) c;
    (void) o;

    SHOW_ERROR( 1, "no op %d", op_no );
    return EINVAL;
}

static pvm_object_t cn_url_blocking_syscall_worker( pvm_object_t conn, struct data_area_4_thread *tc, int nmethod, pvm_object_t arg )
{
    (void) conn;
    (void) tc;
    (void) arg;

    struct data_area_4_connection *c = pvm_object_da( conn, connection );

    struct cn_url_volatile   *vp = c->v_kernel_state;
    struct cn_url_persistent *pp = c->p_kernel_state;

    char buf[4096];

    errno_t e = 0;


    switch( nmethod )
    {
    default:
    case CONN_OP_READ:
        e = net_curl( pp->url, buf, sizeof(buf) );
        break;

    case CONN_OP_WRITE:
    case CONN_OP_BIND:
        e = ENOSYS;
        break;
/*
#if HAVE_NET

        {
            if(!IS_PHANTOM_INT(arg))
            {
                SHOW_ERROR0( 1, "bind arg not int" );
                e = EINVAL;
                break;
            }

            i4sockaddr a;

            a.port = pvm_get_int(arg);

            if( url_bind(vp->url_endpoint, &a) )
                e = EISCONN;
            e = 0;
            break;
        }
#endif // HAVE_NET
*/
    }


ret:
    if( e )
    {
        SHOW_ERROR( 1, "err %d", e );
        return pvm_create_string_object("");
    }


    return pvm_create_string_object(buf);
}


// Init connection
errno_t cn_url_init( struct data_area_4_connection *c, struct data_area_4_thread *t, const char *suffix )
{
    (void) t;

    c->blocking_syscall_worker = cn_url_blocking_syscall_worker;

    SHOW_FLOW( 1, "connect url %s", suffix );

    //struct cn_url_volatile   *vp = c->v_kernel_state;
    struct cn_url_persistent *pp = c->p_kernel_state;

    strlcpy( pp->url, suffix, sizeof(pp->url) );
    /*
#if HAVE_NET
    int rc = url_open( &vp->url_endpoint );
    if( rc )
#endif // HAVE_NET
        return ENOMEM;
*/

    return 0;
}


// TODO make sure it is called on object destruction
// Finish connection
errno_t cn_url_disconnect( struct data_area_4_connection *c )
{
    (void) c;

    SHOW_FLOW0( 1, "disconnect" );
/*
    struct cn_url_volatile *vp = c->v_kernel_state;

    if( !(vp->url_endpoint) )
        return 0;

#if HAVE_NET
    int rc = url_close( vp->url_endpoint );
    if( rc )
#endif // HAVE_NET
        return EINVAL;
*/
    return 0;
}



