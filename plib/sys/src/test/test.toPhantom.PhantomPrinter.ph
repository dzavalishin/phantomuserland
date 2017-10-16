/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Simple console (window) print class
 *
 *
**/


package .test.toPhantom;

import .internal.io.tty;


class PhantomPrinter 
//implements IPhantomPrinter
{
    var console : .internal.io.tty;


	void print(var s : .internal.string )
	{ 
        console.putws(s);
        //console.putws("\n");
	}

	void printInt(var i : .internal.int )
	{ 
        console.putws(i.toString());
        //console.putws("\n");
	}

                      
    void init(var tt : .internal.io.tty ) 
    {
        console = tt;
    }
	

};

