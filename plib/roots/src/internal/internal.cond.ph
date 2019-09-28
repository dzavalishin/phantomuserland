/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Cond.
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

class .internal.cond
{

	void wait( .internal.mutex toUnlock ) [8] {  }
	void twait( .internal.mutex toUnlock, int waitTimeMsec ) [9] {  }

	void broadcast() [10] {  }
	void signal() [11] {  }

};



