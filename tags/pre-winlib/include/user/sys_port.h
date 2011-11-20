#include <phantom_types.h>

#include <newos/portinfo.h>

int	create_port(int q_length, const char *name);
int	close_port(int id);
int	delete_port(int id);
int	find_port(const char *name);
int	get_port_info(int id, struct port_info *pinfo);
int	get_port_bufsize(int id);
int	get_port_bufsize_etc(int, int, int);
int	get_port_count(int id);
int	read_port(int id, int32_t * msg_code, void *msg_buffer, int bufsize);
//int	read_port_etc();
int	write_port(int id, int32_t msg_code, void *msg_buffer, int bufsize);
//int	write_port_etc();
//int	set_port_owner();
//int	get_next_port_info();
