/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal: yes
 * Preliminary: yes
 *
 *
**/

package .internal;

/**
 *
 * This class has internal implementation (as everything in
 * .internal package). It means that VM will bever load its
 * bytecode, and internal version will be used instead. This
 * class definition must be synchronized with VM implementation.
 *
**/

/**
 *
 * Root of all evil. :)
 *
 * TODO: fix return types.
 *
**/

class .internal.object
{

	void construct_me () [0] { }
	void destruct_me () [1] {  }
	string getClass () [2] {  } // wrong - not a string
	void clone() [3] {  }
	int equals(var object) [4] { }

	.internal.object toString() [5] {  }

	int hashCode() [15] { return 0; }

};



