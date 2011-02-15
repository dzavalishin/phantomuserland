#ifndef BOOT_H
#define BOOT_H

extern int main_argc;
extern const char **main_argv;

extern int main_envc;
extern const char **main_env;

extern int boot_argc;
extern const char **boot_argv;

#include <sys/utsname.h>
extern struct utsname phantom_uname;


#endif // BOOT_H
