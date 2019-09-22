/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Hash map
 *
**/

package .phantom.util;

import .phantom.util.hashpair;

/**
 *
 * Map.
 *
**/

class hashmap
{

    var hashed   : hashpair [];
    var hashsize : int; // hash array size - hash val limit


    void init()
    {
        hashsize = 20;
        hashed = new hashpair[]();
    }

//    int pos( .internal.object data )

    int pos( var data : .internal.object )
    {
        return data.hashCode() % hashsize;
    }

    hashpair find( var key : .internal.object, var put : int )
    {
        var bucket : int;
        bucket = pos( key );

        var next : hashpair;
        next = hashed[bucket];

        // pointer != null
        while( next :!= null )
        {
            if( key == next.getKey() )
                return next;

            next = next.getNext();
        }

        // not found
        if( put == 0 )
            return null;

        next = new hashpair();

        next.setNext( hashed[bucket] );
        next.setKey( key );

        hashed[bucket] = next;

    }

    void put( var key : .internal.object, var val : .internal.object )
    {
        var hp : hashpair;

        lock();
        hp = find( key, 1 );
        hp.setVal(val);
        unlock();
    }

    .internal.object get( var key : .internal.object )
    {
        var hp : hashpair;

        lock();
        hp = find( key, 0 );
        unlock();
        if( hp :== null ) return null;
        return hp.getVal();
    }

    // TODO mutex in subclass?
    void lock() {}
    void unlock() {}


};



