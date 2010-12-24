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


//int port_init(kernel_args *ka);
int port_init(void);

// kernel API
port_id		port_create(int32 queue_length, const char *name);
int			port_close(port_id id);
int			port_delete(port_id id);
port_id		port_find(const char *port_name);
int			port_get_info(port_id id, struct port_info *info);
int		 	port_get_next_port_info(proc_id proc,
				uint32 *cookie,
				struct port_info *info);
ssize_t		port_buffer_size(port_id port);
ssize_t		port_buffer_size_etc(port_id port,
				uint32 flags,
				bigtime_t timeout);
int32		port_count(port_id port);
ssize_t		port_read(port_id port,
				int32 *msg_code,
				void *msg_buffer,
				size_t buffer_size);
ssize_t		port_read_etc(port_id port,
				int32 *msg_code,
				void *msg_buffer,
				size_t buffer_size,
				uint32 flags,
				bigtime_t timeout);
int			port_set_owner(port_id port, proc_id proc);
int			port_write(port_id port,
				int32 msg_code,
				void *msg_buffer,
				size_t buffer_size);
int		 	port_write_etc(port_id port,
				int32 msg_code,
				void *msg_buffer,
				size_t buffer_size,
				uint32 flags,
				bigtime_t timeout);
int port_delete_owned_ports(proc_id owner);


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

