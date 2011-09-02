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


class shell //extends runnable
{
    var console : .internal.io.tty;
    var incr :  int;

	var win : .internal.window;
    var conn : .internal.connection;

    void run(var parent_object @const ) [8]
    {
/*
		win = new .internal.window();
		win.setWinPosition(500,310);
		win.setTitle("Window!");
		win.setFg(0); // black
		win.drawBox( 10, 10, 20, 20 );
		win.drawString( 22, 22, "Say Hello to Win!" );
*/
        conn = new .internal.connection();
        conn.connect("timer");

        console = new .internal.io.tty();
        incr = 1;

		console.moveWindow(10,10);
		console.setTitle("VM Shell");

        while(1)
        {
            console.putws("I am connected shell!\n");
            console.putws("--------------------------------------------------\n");
            console.putws("Thread is running... ");
            incr = incr + 1;
            console.putws(incr.5());
            console.putws("  ");
        }
    }
};

