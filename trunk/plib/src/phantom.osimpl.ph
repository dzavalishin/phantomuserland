/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal: No
 * Preliminary: Yes
 *
 *
**/

package .phantom;
import .phantom.os.time;

/**
 *
 * Main OS services implementation.
 * Current implementation is temporary and lets everybody to 
 * replace OS servivce with own implementation. This will be changed.
 *
**/

class osimpl
{
	.phantom.os.time timeServer;

	void setTimeServer( var ts : .phantom.os.time ) { timeServer = ts; }
	.phantom.os.time getTimeServer() { return timeServer; }

};



