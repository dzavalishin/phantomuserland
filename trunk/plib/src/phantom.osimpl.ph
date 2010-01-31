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

        string getKernelEnvironmentValue( var inkey : .internal.string )
        {
            var env : string [];
            env = bootObject.23(); // Kernel env ("key=val") array

            var size : int;
            var i : int;

            size = env.12(); // hack, of course
            i = size;

            while( i > 0 )
            {
                i = i - 1;
                var el : string;
                el = env[i];

                var pos : int;
                pos = el.strstr("=");

                if( pos < 0 )
                    continue;

                var key : string;
                var val : string;

                key = el.substring( 0, pos );
                val = el.substring( pos+1, el.length() - pos - 1 );

                if( key == inkey ) return val;

            }
            return null;
        }

	void setTimeServer( var ts : .phantom.os.time ) { timeServer = ts; }
	.phantom.os.time getTimeServer() { return timeServer; }

};



