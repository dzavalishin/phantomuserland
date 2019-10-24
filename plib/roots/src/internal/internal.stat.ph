/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel statistics access
 *
 *
 * This class has internal implementation (as everything in
 * .internal package). It means that VM will never load its
 * bytecode, and internal version will be used instead. This
 * class definition must be synchronized with VM implementation.
 *
**/

package .internal;

/**
 *
**/


class .internal.stat
{

	// Call URL and return http server reply
	//.internal.string curl( var host : .internal.string, var headers : .internal.string ) [24] {  }

	int getStat( var nCounter : int, var kind : int ) [16] {}
	int getIdle(  ) [17] {}


};



