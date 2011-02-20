#include <errno.h>

    int    gettid(void);
    int    getpid(void);
    int    getuid(void);
    int    getuid32(void);
    int    getpgid(int pid);
    int    getpgrp(void);
    int    getppid(void);
    int    getegid(void);
    int    getegid32(void);
    int    geteuid(void);
    int    geteuid32(void);
    int    getgid(void);
    int    getgid32(void);


    // Secod val is to set to
    errno_t setgid(int);
    errno_t setgid32(int);
    errno_t setgroups(int);
    errno_t setgroups32(int);
    errno_t setpgid(int pid, int pgid);
    errno_t setpgrp(void);
    errno_t setregid(int);
    errno_t setregid32(int);
    errno_t setresgid(int);
    errno_t setresgid32(int);
    errno_t setresuid(int);
    errno_t setresuid32(int);
    errno_t setreuid(int);
    errno_t setreuid32(int);
    errno_t setuid(int);
    errno_t setuid32(int);

    int     tcgetpgrp(int __fildes );
    int     tcsetpgrp(int __fildes, int __pgrp_id );

