/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Hash map helper class
 *
**/

package .phantom.util;

class hashpair
{
    var key : .internal.object;
    var val : .internal.object;
    var next : hashpair;

    void setKey( var _key : .internal.object )
    {
        key = _key;
    }

    .internal.object getKey(  )
    {
        return key;
    }

    void setVal( var _val : .internal.object )
    {
        val = _val;
    }

    .internal.object getVal(  )
    {
        return val;
    }


    void setNext( var _n : hashpair )
    {
        next = _n;
    }

    hashpair getNext(  )
    {
        return next;
    }

};
