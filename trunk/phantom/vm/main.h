/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: no
 * Preliminary: yes
 *
 * This source file implements Windows based wrapper for VM to
 * run in Windows-hosted environment.
 *
**/

int load_code(void **out_code, unsigned int *out_size, const char *fn);
void save_mem( void *addr, int size );


void setDiffMem( void *mem, void *copy, int size );
void checkDiffMem();

