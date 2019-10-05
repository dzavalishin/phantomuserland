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

import .internal.mutex;

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

	void wait( var toUnlock : .internal.mutex ) [8] {  }
	void twait( var toUnlock : .internal.mutex, var waitTimeMsec : int ) [9] {  }

	void broadcast() [10] {  }
	void signal() [11] {  }

};



