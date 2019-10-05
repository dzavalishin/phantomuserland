/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Map.
 * Current implementation is temporary and VERY inefficient.
 * Redo ASAP.
 *
 * There is .internal.directory exist.
 *
**/


package .phantom.util;

class map
{
    var pairs : .internal.object [];
    var index : int;

    void init()
    {
        index = 0;
        pairs = new .internal.object[]();
    }

	void map()
	{
		// TODO remove init, use me
	}

    // TODO! Synchronized!
    /** return 0 if already have this one */
    int put( var key : .internal.object, var value : .internal.object )
    {
        // :!= is reference not equal
        if( get( key ) :!= null ) // not null?
            return 0;

        pairs[index] = key;
        pairs[index+1] = value;
        index = index + 2;

        return 1;
    }

    // TODO! Synchronized!
    /** return found value or null */
    .internal.object get( var key : .internal.object )
    {
        var i : .internal.int;
        i = 0;

        while( i < index )
        {
            if( key.equals( pairs[i] ) )
            {
                return pairs[i+1];
            }
            i = i + 2;
        }

        return null;
    }


};



