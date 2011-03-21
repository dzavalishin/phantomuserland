/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Boot time parameters and flags.
 *
 *
**/

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

extern int debug_boot_pause;
extern int bootflag_no_vesa;
extern int bootflag_no_comcon;


#endif // BOOT_H
