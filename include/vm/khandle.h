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

//errno_t  object_assign_handle( ko_handle_t *h, pvm_object_t o );
//errno_t  object_revoke_handle( ko_handle_t h, pvm_object_t o );


// These two work after handle is assigned
errno_t  object2handle( ko_handle_t *h, pvm_object_t o );
errno_t  handle2object( pvm_object_t *o, ko_handle_t h );

// Dec in-kernel refcount incremented by handle2object
errno_t  handle_release_object( ko_handle_t *h );


// -----------------------------------------------------------------------
// General object land interface
// -----------------------------------------------------------------------

errno_t ko_make_int( ko_handle_t *ret, int i );
errno_t ko_make_string( ko_handle_t *ret, const char *s );
errno_t ko_make_string_binary( ko_handle_t *ret, const char *s, int len );

errno_t ko_make_object( ko_handle_t *ret, ko_handle_t ko_class, void *init ); // C'tor args? Or call it manually?

errno_t ko_get_class( ko_handle_t *ret, const char *class_name );

//! Get method ordinal by name - BUG - what if class is reloaded? Or we are tied to class version and pretty sure?
errno_t ko_map_method( int *ret, ko_handle_t *ko_this, const char *method_name );

//! Sync call - returns value - no, will hang snaps!
//ko_handle_t ko_call_method( ko_handle_t *ko_this, int method, int nargs, ko_handle_t *args, int flags );

//! Async call - void, does not wait
void ko_spawn_method( ko_handle_t *ko_this, int method, int nargs, ko_handle_t *args, int flags );

//! Async call - void, does not wait, calls calback
void ko_spawn_method_callback( ko_handle_t *ko_this, int method, int nargs, ko_handle_t *args, int flags, void (*callback)( void *cb_arg, ko_handle_t *ret ), void *cb_arg );


//#define KO_CALL_FLAG_NEW_THREAD (1<<0) - no, spawn instead



