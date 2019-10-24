/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * How to access world. Gives access to own thread object 
 * which, in turn, can give access to global OS services.
 *
 * TODO: directly access some really global things?
 *
 *
 *
 * This class has internal implementation (as everything in
 * .internal package). It means that VM will never load its
 * bytecode, and internal version will be used instead. This
 * class definition must be synchronized with VM implementation.
 *
**/

package .internal;

import .internal."thread";
import .phantom.runnable;

class .internal.world
{

	.internal."thread" getMyThread() [8] {  }


	// returns errno
	//int startThread( var entry : .phantom.runnable, var arg : .internal.object ) [9] {  }
	//int startThread( var entry : .phantom.runnable, var arg : .internal.string ) [9] {  }

	// TEMP TODO fix me - needs .phantom.runnable to be compiled first
	int startThread( var entry : .internal.object, var arg : .internal.object ) [9] {  }

	// Print to system logging facility - practically it is a log window in real OS and stdout in pvm_*.exe
	void log( var msg : .internal.string ) [10] {  }

};



