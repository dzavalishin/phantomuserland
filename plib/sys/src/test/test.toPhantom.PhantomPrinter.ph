/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal: No
 * Preliminary: Yes
 *
 *
**/


package .test.toPhantom;

import .internal.io.tty;

/**
 *
 * Simple console (window) print class
 *
**/

class PhantomPrinter 
//implements IPhantomPrinter
{
    var console : .internal.io.tty;


	void print(var s : .internal.string ) [8]
	{ 
        console.putws(s);
        console.putws("\n");
	}

                      
    void init(var tt : .internal.io.tty ) 
    {
        console = tt;
    }
	

};

