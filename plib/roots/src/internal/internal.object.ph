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

import .internal."class";

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
 * Root of all evil. :)
 *
**/

class .internal.object
{

//	void construct_me () [0] { } 
// when someone explicitly tells that we're base class, compiler wants us to have a constructor - NB - does it really help?
	void object () [0] { }

	void destruct_me () [1] {  }
	.internal."class" getClass () [2] {  }
	void clone() [3] {  }

//	int equals(var object) [4] { }
	int equals(var object: .internal.object) [4] { }

	.internal.string toString() [5] {  }

	int hashCode() [15] { return 0; }

    // have hack in compiler which 'finds' method 0 as default c'tor for us
	//void object() { construct_me(); } // Or else compiler fails to find root default constructor

	void setImmutable() [14] { }

};



