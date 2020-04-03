/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Simple map class inplemented in C.
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

/**
 *
 * Directory. Actually an hashmap with string (or arbitrary binary) as key.
 *
**/

class .internal.directory
{
	int equals(var object) [4] { }


// TODO key can be of any type, modify syscall code and remove type here
	int put( var key : .internal.string, var value )	[8] {}
	.internal.object get( var key ) 			[9] {}

	int remove( var key : .internal.string  ) 	        [10] {}

	int size() 						[11] {}

        // iterator iterate() [12] {}
};



