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

#include <vm/object.h>
#include <kernel/json.h>

pvm_object_t pvm_convert_json_to_objects( json_value *jv );
pvm_object_t pvm_convert_json_to_objects_ext( const char *name, json_value *jv, int tab );

errno_t pvm_print_json( pvm_object_t root );
errno_t pvm_print_json_ext( pvm_object_t key, pvm_object_t o, int tab );

pvm_object_t pvm_json_parse( const char *json );
pvm_object_t pvm_json_parse_ext( const char *json, size_t json_len );


#endif // VM_JSON_H
