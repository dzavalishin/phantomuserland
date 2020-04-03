/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Mutex.
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

class .internal.mutex
{

	void lock() [8] {  }
	void unlock() [9] {  }

	// unimpl
	//void trylock() [10] {  }

};



