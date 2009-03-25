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
import .phantom.osimpl;
import .internal.world;
import .internal."thread";

/**
 *
 * Main OS services interface.
 * Current implementation is temporary and lets everybody to 
 * replace OS servivce with own implementation. This will be changed.
 *
**/

class os
{
    .phantom.osimpl impl;

    void init()
    {
        if( impl :!= null )
	{
            var w : .internal.world;
            w = new .internal.world();

            var th : .internal."thread";
            th = w.getMyThread();

            impl = th.getOsInterface();
	}
    }


    .phantom.os.time getTimeServer() { init(); return impl.getTimeServer(); }

};



