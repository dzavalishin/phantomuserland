/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Time related syscalls.
 * 
 *
 * This class has internal implementation (as everything in
 * .internal package). It means that VM will never load its
 * bytecode, and internal version will be used instead. This
 * class definition must be synchronized with VM implementation.
 *
**/

package .internal;



class .internal.time
{

//	.internal."class" getClass () [2] {  }

	// classic Unix 32 bit time. TODO make it to be 64?
	.internal.int unixTime() [16] {  }

	void sleepSec( var timeSec : int ) [17] {}
	void sleepMsec( var timeMsec : int ) [18] {}

	// void runLater( var start : .phantom.runnable, var timeMsec : int );

};



