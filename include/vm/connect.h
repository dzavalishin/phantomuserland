/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel/VM connection definitions.
 *
 *
**/

#ifndef VM_CONNECT_H
#define VM_CONNECT_H

#include <vm/internal_da.h>
#include <errno.h>
#include <sys/types.h>

//! Call connection's callback with binary payload
errno_t phantom_connection_callback_binary( struct data_area_4_connection *da, void *data, size_t size );

//! Call connection's callback with string payload
errno_t phantom_connection_callback_string( struct data_area_4_connection *da, const char *data );

//! Call connection's callback with int payload
errno_t phantom_connection_callback_int( struct data_area_4_connection *da, int data );


// Specific ones


#define CON_F_PROTOS(__short) \
errno_t cn_##__short##_do_operation( int op_no, struct data_area_4_connection *c, struct data_area_4_thread *tc, pvm_object_t o ); \
errno_t cn_##__short##_init( struct data_area_4_connection *c, struct data_area_4_thread *tc, const char *suffix ); \
errno_t cn_##__short##_disconnect( struct data_area_4_connection *c );

#define CON_F_NAMES(__short) \
 { cn_##__short##_do_operation, cn_##__short##_init, cn_##__short##_disconnect }


CON_F_PROTOS(timer)
CON_F_PROTOS(stats)
CON_F_PROTOS(udp)
CON_F_PROTOS(fio)       // File IO
CON_F_PROTOS(url)       // curl style HTTP


// Well known connection operation numbers. Not all types of conns use these numbers.

#define CONN_OP_READ            0
#define CONN_OP_WRITE           1
#define CONN_OP_SEEK            2


#define CONN_OP_BIND           16


struct cn_udp_persistent
{
    int fill;
};

struct cn_udp_volatile
{
    void *udp_endpoint;
    //int fill;
};

#include <unix/uufile.h>

struct cn_fio_persistent
{
    char filename[FS_MAX_PATH_LEN];
};

struct cn_fio_volatile
{
    //void *udp_endpoint;
    int fd;
};





struct cn_url_persistent
{
    char url[FS_MAX_PATH_LEN];
};

struct cn_url_volatile
{
    errno_t status;
    //void *tcp_endpoint;
};



#endif // VM_CONNECT_H
