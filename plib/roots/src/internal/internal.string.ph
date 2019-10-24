/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * String. Can contain any binary data and 
 * sometimes is used as binary buffer.
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

import .internal.int;
import .internal.long;


class .internal.string
{
	int equals(var object) [4] { }

	.internal.string substring( var index : int, var length : int ) [8] {}

    // todo short version of substring 

	int charAt( var index : int ) [9] {}

	.internal.string concat( var s : string ) [10] {}

	int length() [11] {}

	int strstr( var s : string ) [12] {}

	void setImmutable() [14] { }

    int  toInt() [16] { }
    long toLong() [17] { }

    //float  toFloat() [18] { }
    //double toDouble() [19] { }


	.internal.object parseJson() [20] { } // returns tree of arrays/directories/leaf objects

};



