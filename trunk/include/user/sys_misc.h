#include <phantom_types.h>

extern void _exit(int);
extern int getpagesize();
extern int personality(int kind);

extern int gethostname(const char *buf, int bs);

extern int waitpid( int pid, int *status, int options);

//errno_t name2ip( in_addr_t *out, const char *name, int flags );

extern int kill(int pid);

extern void sync(void);
