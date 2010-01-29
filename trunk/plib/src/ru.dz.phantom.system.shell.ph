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
import .ru.dz.phantom.system.runnable;

attribute const * ->!;


class shell extends runnable
{
    var console : .internal.io.tty;
    var incr :  int;

    void run(var parent_object @const ) [8]
    {
        console = new .internal.io.tty();
        incr = 1;

        while(1)
        {
            console.putws("Thread is running... ");
            console.putws("... ");
            incr = incr + 1;
            console.putws(incr.5());
            console.putws("  ");
        }
    }
};

