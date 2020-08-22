/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Helpers to access VM objects from C
 *
 *
**/

#ifndef P2C_H
#define P2C_H

#include "vm/internal_da.h"

#define IS_PHANTOM_INT(obj) (obj->_class == pvm_get_int_class())
#define IS_PHANTOM_STRING(obj) (obj->_class == pvm_get_string_class())

#define EQ_STRING_P2C(obj,cstring) ((((unsigned)pvm_get_str_len(obj))==strlen((const char *)cstring))&&(0==strncmp((const char *)pvm_get_str_data(obj),(const char *)cstring,pvm_get_str_len(obj))))

#define pvm_get_str_len( o )  ( (int) (((struct data_area_4_string *)&(o->da))->length))
#define pvm_get_str_data( o )  ( (char *) (((struct data_area_4_string *)&(o->da))->data))


#endif // P2C_H
