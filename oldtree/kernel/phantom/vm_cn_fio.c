/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2015 Dmitry Zavalishin, dz@dz.ru
 *
 * Connection to File IO.
 *
**/

#define DEBUG_MSG_PREFIX "vmcn.fio"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <vm/connect.h>
#include <vm/alloc.h>
#include <vm/p2c.h>
#include <vm/syscall.h>

// kernel unix-like file io
#include <kunix.h>
#include <sys/fcntl.h>


errno_t cn_fio_do_operation( int op_no, struct data_area_4_connection *c, struct data_area_4_thread *t, pvm_object_t o )
{
    (void) t;
    (void) c;
    (void) o;

    SHOW_ERROR( 1, "no op %d", op_no );
    return EINVAL;
}

static pvm_object_t cn_fio_blocking_syscall_worker( pvm_object_t conn, struct data_area_4_thread *tc, int nmethod, pvm_object_t arg )
{
    (void) conn;
    (void) tc;
    (void) arg;

    struct data_area_4_connection *c = pvm_object_da( conn, connection );
    struct cn_fio_volatile *vp = c->v_kernel_state;



    errno_t e = ENOSYS;
    int ret = 0;


    //if( !vp->fio_endpoint )
    if( vp->fd < 0 )
    {
        e = ENOTCONN;
        goto ret;
    }


    switch( nmethod )
    {
    default:

    case CONN_OP_SEEK:
        if( !IS_PHANTOM_INT(arg) )
        {
            SHOW_ERROR0( 1, "seek arg is not integer" );
            pvm_object_dump( arg );
            e = EINVAL;
            break;
        }
        else
        {
            int pos = pvm_get_int(arg);
            int opos;

            SHOW_FLOW( 1, "seek %d", pos );
            e = k_seek( &opos, vp->fd, pos, 0 ); // BUG FIXME last 0 is SEEK_SET

            if( e )
            {
                SHOW_FLOW( 1, "seek failed , errno=%d", e );
                ret = e;
                break;
            }
            //if( !e && (nread >= 0) )                ret = nwritten;
            SHOW_FLOW( 1, "seek opos %d", opos );
            //return pvm_create_string_object_binary( buf, nread );
        }

        break;


    case CONN_OP_READ:
        //e = ENOSYS;

        if( !IS_PHANTOM_INT(arg) )
        {
            SHOW_ERROR0( 1, "read arg is not integer" );
            pvm_object_dump( arg );
            e = EINVAL;
            break;
        }
        else
        {
            int len = pvm_get_int(arg);
            int nread = 0;

            char buf[len];

            SHOW_FLOW( 1, "read %d", len );

            e =  k_read( &nread, vp->fd, buf, len );
            if( e )
            {
                SHOW_FLOW( 1, "read failed , errno=%d", e );
                ret = e;
                break;
            }
            //if( !e && (nread >= 0) )                ret = nwritten;
            SHOW_FLOW( 1, "read %d '%*s'", nread, nread, buf );
            return pvm_create_string_object_binary( buf, nread );
        }

        break;

    case CONN_OP_WRITE:
        if( !IS_PHANTOM_STRING(arg) )
        {
            e = EINVAL;
            break;
        }
        else
        {
            int len = pvm_get_str_len(arg);
            const char *data = (const char *)pvm_get_str_data(arg);
            int nwritten = 0;

            e =  k_write( &nwritten, vp->fd, data, len );
            //if( !e && (nwritten != len) )                e = EIO;
            if( !e && (nwritten >= 0) )
                ret = nwritten;
            SHOW_FLOW( 1, "write %d '%*s'", nwritten, len, data );
        }

        break;

    case CONN_OP_BIND:
        SHOW_ERROR0( 1, "no bind for fio" );
        e = ENOSYS;
        break;

    }


ret:
    if( e )
    {
        SHOW_ERROR( 1, "err %d", e );
        return pvm_create_int_object(-e);
    }


    return pvm_create_int_object(ret);
}


// Init connection
errno_t cn_fio_init( struct data_area_4_connection *c, struct data_area_4_thread *t, const char *suffix )
{
    (void) t;

    c->blocking_syscall_worker = cn_fio_blocking_syscall_worker;

    SHOW_FLOW( 1, "connect fio %s", suffix );

    struct cn_fio_volatile *vp = c->v_kernel_state;

    vp->fd = -1;
#if HAVE_UNIX
    errno_t rc = k_open( &vp->fd, suffix, O_RDWR|O_CREAT, 0666 ); // TODO flags/mode?
    if( rc )
    {
        SHOW_ERROR( 1, "connect fio '%s' rc=%d", suffix, rc );
        return rc;
    }
#endif // HAVE_UNIX

    return 0;
}


// TODO make sure it is called on object destruction
// Finish connection
errno_t cn_fio_disconnect( struct data_area_4_connection *c )
{
    (void) c;

    SHOW_FLOW0( 1, "disconnect" );

    struct cn_fio_volatile *vp = c->v_kernel_state;

    if( vp->fd < 0 )
        return 0;

#if HAVE_UNIX
    errno_t rc = k_close( vp->fd );
    if( rc )
        return rc;
#endif // HAVE_UNIX

    return 0;
}




