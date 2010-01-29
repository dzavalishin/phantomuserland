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

/**
 *
 * Main OS services implementation.
 * Current implementation is temporary and lets everybody to 
 * replace OS servivce with own implementation. This will be changed.
 *
**/

class osimpl
{
        .internal.object bootObject;
	.phantom.os.time timeServer;

        void init(var _bootObject : .internal.object ) { bootObject = _bootObject; }

        string getEnvironmentValue( var key : .internal.string )
        {
            var env : string [];
            env = bootObject.23();

            var size : int;

            size = env.12(); // hack, of course

            while( size > 0 )
            {
                size = size - 1;
                var el : string;
                el = env[size];


            }
            return null;
        }

	void setTimeServer( var ts : .phantom.os.time ) { timeServer = ts; }
	.phantom.os.time getTimeServer() { return timeServer; }

};



