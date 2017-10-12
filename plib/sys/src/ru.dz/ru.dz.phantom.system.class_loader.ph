/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal: no
 * Preliminary: yes
 *
 *
**/

/**
 *
 * Class loader, the most basic implementation. Uses kernel boot module
 * to load classes, caches everything in simple array. Slow, dumb,
 * have to be rewritten with more fast cache storage (tree?), must be
 * able to load from network.
 *
 * Supposed to:
 *
 *  1. On first call (system installation) attempt to load all
 *     the classes it will need to be able to access network.
 *
 *  2. Load classes from network when boot (install) class loader
 *     is not available anymore.
 *
 * Problem:
 *
 *  At this moment class loader is started from kernel and locks up 
 *  everything (namely - snaps) while is working.
 *
**/


package .ru.dz.phantom.system;



attribute const * ->!;

class class_loader
{
    var classes : .internal.object [];
    var index : int;
    var boot_object;

    // locals
    var new_class;
    var i : int;

    void init( var _boot_object )
    {
        boot_object = _boot_object;
        index = 0;

        classes = new void[]();
    }

    void print( var input : string )
    {
        boot_object.16(input);
    }

    .internal.object load_class( var name : string )
    {
        return boot_object.8(name);
    }

    .internal.object load( var name : string  )
    {
        new_class = load_class(name);

        classes[index] = name;
        index = index + 1;
        classes[index] = new_class;
        index = index + 1;

        return new_class;
    }


    .internal.object get_class( var name : string ) [8]
    {
        //print("Loader: class '"); print(name); print("' requested... ");
        i = 0;

        while( i < index )
        {
            if( name == classes[i] )
            {
                //print("and found\n");
                return classes[i+1];
            }
            i = i + 2;
        }

        //print("loading\n");
        return load(name);
    }

};





