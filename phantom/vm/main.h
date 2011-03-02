/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 *
 * This source file implements Windows based wrapper for VM to
 * run in Windows-hosted environment.
 *
**/

void save_mem( void *addr, int size );


void setDiffMem( void *mem, void *copy, int size );
void checkDiffMem(void);


int load_class_from_file(const char *cn, struct pvm_object *out);

