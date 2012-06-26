#ifndef SYS_PHANTOM
#define SYS_PHANTOM

#include <phantom_types.h>
#include <errno.h>

// Syscall will not return until process exit - ugly, but handy ;)
#define P_RUN_WAIT  	        (1<<0)

// Run in a new window
#define P_RUN_NEW_WINDOW    	(1<<1)

// Process group (signal handling)
#define P_RUN_NEW_PGROUP        (1<<2)

// returns pid or -1
extern int phantom_run(const char *fname, const char **argv, const char **envp, int flags);



extern errno_t phantom_runclass(const char *cname, int nmethod, int flags);


#endif // SYS_PHANTOM
