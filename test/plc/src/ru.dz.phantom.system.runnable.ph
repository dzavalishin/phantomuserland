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


package .ru.dz.phantom.system;

attribute const * ->!;

/**
 *
 * Runnable. Place to hardcode run method ordinal.
 *
**/

class runnable
{
	// They call us here
	void run(var parent_object @const ) [8]
	{ 
		throw "Abstract runnable started";
	}

};







