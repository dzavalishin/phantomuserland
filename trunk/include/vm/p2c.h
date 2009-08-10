/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes?
 *
 *
**/

#ifndef P2C_H
#define P2C_H

#include "vm/internal_da.h"

//#define IS_PHANTOM_STRING(obj) (obj.my_data()->class_is(pvm_object_storage::get_string_class()))
//#define IS_PHANTOM_INT(obj) (obj.my_data()->class_is(pvm_object_storage::get_int_class()))

#define IS_PHANTOM_INT(obj) (obj.data->_class.data == pvm_get_int_class().data)
#define IS_PHANTOM_STRING(obj) (obj.data->_class.data == pvm_get_string_class().data)

#define EQ_STRING_P2C(obj,cstring) ((pvm_get_str_len(obj)==strlen((const char *)cstring))&&(0==strncmp((const char *)pvm_get_str_data(obj),(const char *)cstring,pvm_get_str_len(obj))))

#endif // P2C_H
