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
 * This is needed to compile all classes from scratch. Real
 * .internal.object refers to .internal.string, so we compile this first,
 * then compile .internal.string, then REAL .internal.object, and then
 * .internal.string again so that it will have all the methods of
 * .internal.object too.
 *
**/

/**
 *
 * Root of all evil. :)
 *
**/

class .internal.object
{

	void construct_me () [0] { }
	void destruct_me () [1] {  }

	.internal.object getClass () [2] {  } // wrong

	void clone() [3] {  }
	int equals(var object) [4] { }

	.internal.object toString() [5] {  } // wrong

	int hashCode() [15] { return 0; }

    // have hack in compiler which 'finds' method 0 as default c'tor for us
	//void object() { construct_me(); } // Or else compiler fails to find root default constructor

};



