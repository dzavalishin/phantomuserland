#include <sys/types.h>
#include <newos/compat.h>

typedef int port_id;
typedef int proc_id;

struct port_info {
	port_id id;
	proc_id owner;
	char name[SYS_MAX_OS_NAME_LEN];
	int32_t capacity;
	int32_t queue_count;
	int32_t total_count;
};
