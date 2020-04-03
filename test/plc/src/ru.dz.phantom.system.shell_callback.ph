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

package .ru.dz.phantom.system;

import .phantom.os;
import .internal.io.tty;
import .internal.window;
import .internal.connection;
import .ru.dz.phantom.system.runnable;

attribute const * ->!;


class shell_callback //extends runnable
{
    var console : .internal.io.tty;

    void run(var time : int ) [17]
    {
        console.putws("\n\nTimer callback: ");
        console.putws(time.toString());
        console.putws(" !!\n\n");
    }
                      
    void init(var tt : .internal.io.tty ) 
    {
        console = tt;
    }


};
