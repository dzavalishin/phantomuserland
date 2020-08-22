/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Some compiler/VM regression tests.
 *
 *
 **/

package .ru.dz.phantom.system;

attribute const * ->!;


class regression_tests_ctor
{
	var saved : .internal.int;

	void regression_tests_ctor( var a : .internal.int )
	{
		saved = a;
	}

	void check( var b : .internal.int )
	{
        if( b != saved ) 		throw "constructor with parameters failed";
	}
};


