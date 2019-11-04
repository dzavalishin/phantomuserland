/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * StringBuilder. Mutable string.
 *
 * When used in a string context, treated as UTF-8.
 *
 *
 * This class has internal implementation (as everything in
 * .internal package). It means that VM will never load its
 * bytecode, and internal version will be used instead. This
 * class definition must be synchronized with VM implementation.
 *
**/

package .internal;


class .internal.stringbuilder
{
	int equals(var object) [4] { }

	.internal.string toString(  ) [5] {}

	.internal.string substring( var index : int, var length : int ) [8] {}

    // todo short version of substring 

	int byteAt( var index : int ) [9] {}

	int concat( var s : string ) [10] {}

	int length() [11] {}

	int strstr( var s : string ) [12] {}

};



