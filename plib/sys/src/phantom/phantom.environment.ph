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

import .phantom.util.map;

/**
 *
 * Environment: set of objects mapped to string names.
 * Used as kernel and thread environment.
 *
**/

class environment
{
    // TODO! Constructor!
    .phantom.util.map env;

    void init()
    {
        // TODO why :== ?
        if( env :== null )
            env = new .phantom.util.map();
    }

    int set( var key: .internal.string, var val  : .internal.object )
    {
        init();
        return env.put(key, val );
    }

    .internal.object get(var key: .internal.string )
    {
        init();
        return env.get(key);
    }

};



