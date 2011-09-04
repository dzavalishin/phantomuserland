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
import .ru.dz.phantom.system.shell_callback;

attribute const * ->!;

/*
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
*/

class shell //extends runnable
{
    var console : .internal.io.tty;
    var incr :  int;

	var win : .internal.window;
    var conn : .internal.connection;

    var cb : shell_callback;

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

        console = new .internal.io.tty();
        incr = 1;

		console.moveWindow(10,10);
		console.setTitle("VM Shell");

        // test of connections
        conn = new .internal.connection();
        conn.connect("tmr:");

        cb = new shell_callback();
        cb.init( console );

	    conn.setCallback( cb, 17 );

	    conn.invoke( 1000, 0 ); // op 0 - set timer, arg - msecs



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

