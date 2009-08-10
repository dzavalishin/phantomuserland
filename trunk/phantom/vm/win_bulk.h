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

#include <vm/bulk.h>



int bulk_seek_f( int pos );
int bulk_read_f( int count, void *data );

extern void *bulk_code;
extern unsigned int bulk_size;
extern void *bulk_read_pos;

