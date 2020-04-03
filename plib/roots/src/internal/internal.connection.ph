/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * General userland-kernel connection object.
 *
 *
**/

package .internal;

/**
 *
 * This class has internal implementation (as everything in
 * .internal package). It means that VM will never load its
 * bytecode, and internal version will be used instead. This
 * class definition must be synchronized with VM implementation.
 *
**/

class .internal.connection
{
	int equals(var object) [4] { }

	int setCallback( var callback_object, var n_method : int ) [12] {}
	int connect( var destination : string ) [8] {}
	int disconnect(  ) [9] {}

	int invoke( var object, var operation_id : int ) [11] {}

	int block( var object, var operation_id : int ) [13] {}
	//.internal.object block( var object, var operation_id : int ) [13] {}

};



