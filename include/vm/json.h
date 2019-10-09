/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * VM JSON support
 *
 *
**/

#ifndef VM_JSON_H
#define VM_JSON_H

#include "vm/object.h"

errno_t pvm_print_json( pvm_object_t root );
errno_t pvm_print_json_ext( pvm_object_t key, pvm_object_t o, int tab );

#endif // VM_JSON_H
