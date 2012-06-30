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

#include <arch/arch-flags.h>


//! Fast access to frequently needed information about 
//! current machine - see arch/arch-flags.h.
//! These flags must be set before main() starts - in
//! arch_init_early() / board_init_early() / detect_cpu()

extern unsigned int	arch_flags; // in multiboot.c

#define ARCH_HAS_FLAG(__reqired_flags) ((__reqired_flags)&arch_flags)
#define ARCH_SET_FLAG(__flags) (arch_flags |= (__flags))

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
extern int bootflag_unattended;


#endif // BOOT_H
