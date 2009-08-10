/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: yes
 *
**/

#ifndef BULK_H
#define BULK_H

#include <phantom_types.h>


typedef int (*pvm_bulk_read_t)(int count, void *data);
typedef int (*pvm_bulk_write_t)(int count, const void *data);
typedef int (*pvm_bulk_seek_t)(int pos);

#define PVM_BULK_CN_LENGTH 512

struct pvm_bulk_class_head
{
    char        name[PVM_BULK_CN_LENGTH];
    u_int32_t   data_length;
};

int pvm_load_class_from_module( char *class_name, struct pvm_object   *out );
int pvm_load_class_from_memory( unsigned char * contents, int fsize, struct pvm_object* out );
void pvm_bulk_init( pvm_bulk_seek_t sf, pvm_bulk_read_t rd );






#endif // BULK_H

