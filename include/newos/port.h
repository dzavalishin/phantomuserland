/* 
** Copyright 2001-2002, Mark-Jan Bastian. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef _NEWOS_PORT_H
#define _NEWOS_PORT_H

/*
 *	beos-style kernel ports
 *	provides a queue of msg'code and variable-length data
 *	that can be used by any two threads to exchange data
 *	with each other
 */

//#include <boot/stage2.h>
#include <newos/compat.h>
#include <newos/types.h>

// PORT_FLAG_INTERRUPTABLE must be the same as SEM_FLAG_INTERRUPTABLE
// PORT_FLAG_TIMEOUT       must be the same as SEM_FLAG_TIMEOUT
#define PORT_FLAG_TIMEOUT 2
#define PORT_FLAG_INTERRUPTABLE 4
#define PORT_FLAG_USE_USER_MEMCPY 0x80000000

/*
typedef int port_id;
typedef int proc_id;

struct port_info {
	port_id id;
	proc_id owner;
	char name[SYS_MAX_OS_NAME_LEN];
	int32 capacity;
	int32 queue_count;
	int32 total_count;
};
*/

#include <newos/portinfo.h>

#include <threads.h>
#include <errno.h>



errno_t         phantom_port_init(void);

// -----------------------------------------------------------------------
//
// Kernel API
//
// -----------------------------------------------------------------------

//! Deletes ports owned by 'owner', 'count' can be 0
errno_t 		phantom_port_delete_owned_ports(tid_t owner, int *count);


errno_t         phantom_port_create(port_id *ret, int32 queue_length, const char *name);
errno_t         phantom_port_close(port_id id);
errno_t         phantom_port_delete(port_id id);
errno_t	        phantom_port_find(port_id *ret, const char *port_name);
errno_t         phantom_port_get_info(port_id id, struct port_info *info);
errno_t         phantom_port_get_next_port_info(proc_id proc,
					uint32 *cookie,
					struct port_info *info);
errno_t			phantom_port_buffer_size(ssize_t *sizep, port_id port);
errno_t			phantom_port_buffer_size_etc(ssize_t *sizep, port_id port,
					uint32 flags,
					bigtime_t timeout);
errno_t			phantom_port_count(int32 *countp, port_id port);
errno_t			phantom_port_read(ssize_t *len, 
					port_id port,
					int32 *msg_code,
					void *msg_buffer,
					size_t buffer_size);
errno_t			phantom_port_read_etc(ssize_t *len, 
					port_id port,
					int32 *msg_code,
					void *msg_buffer,
					size_t buffer_size,
					uint32 flags,
					bigtime_t timeout);
int				phantom_port_set_owner(port_id port, proc_id proc);
errno_t			phantom_port_write(port_id port,
					int32 msg_code,
					void *msg_buffer,
					size_t buffer_size);
errno_t	 		phantom_port_write_etc(port_id port,
					int32 msg_code,
					void *msg_buffer,
					size_t buffer_size,
					uint32 flags,
					bigtime_t timeout);



// temp: test
//void port_test(void);
//int	 port_test_thread_func(void* arg);

/*
// user-level API
port_id		user_port_create(int32 queue_length, const char *name);
int			user_port_close(port_id id);
int			user_port_delete(port_id id);
port_id		user_port_find(const char *port_name);
int			user_port_get_info(port_id id, struct port_info *info);
int		 	user_port_get_next_port_info(proc_id proc,
				uint32 *cookie,
				struct port_info *info);
ssize_t     user_port_buffer_size(port_id port);
ssize_t		user_port_buffer_size_etc(port_id port,
				uint32 flags,
				bigtime_t timeout);
int32		user_port_count(port_id port);
ssize_t 	user_port_read(port_id uport, int32 *umsg_code, void *umsg_buffer,
				size_t ubuffer_size);
ssize_t		user_port_read_etc(port_id port,
				int32 *msg_code,
				void *msg_buffer,
				size_t buffer_size,
				uint32 flags,
				bigtime_t timeout);
int			user_port_set_owner(port_id port, proc_id proc);
int			user_port_write(port_id uport, int32 umsg_code, void *umsg_buffer,
				size_t ubuffer_size);
int			user_port_write_etc(port_id port,
				int32 msg_code,
				void *msg_buffer,
				size_t buffer_size,
				uint32 flags,
				bigtime_t timeout);

*/

#endif // _NEWOS_PORT_H

