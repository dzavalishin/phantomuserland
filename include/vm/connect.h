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
errno_t cn_##__short##_init( struct data_area_4_connection *c, struct data_area_4_thread *tc ); \
errno_t cn_##__short##_disconnect( struct data_area_4_connection *c, struct data_area_4_thread *tc );

#define CON_F_NAMES(__short) \
 { cn_##__short##_do_operation, cn_##__short##_init, cn_##__short##_disconnect }


CON_F_PROTOS(timer)


#endif // VM_CONNECT_H
