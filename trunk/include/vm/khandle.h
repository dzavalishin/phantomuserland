/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Map object reference to handle and vice versa.
 * Handle makes sure that object exists as long as
 * kernel needs it.
 *
**/

#include <errno.h>

typedef int ko_handle_t;

errno_t  object_assign_handle( ko_handle_t *h, pvm_object_t o );
errno_t  object_revoke_handle( ko_handle_t h, pvm_object_t o );


// These two work after handle is assigned
errno_t  object2handle( ko_handle_t *h, pvm_object_t o );
errno_t  handle2object( pvm_object_t *o, ko_handle_t h );

