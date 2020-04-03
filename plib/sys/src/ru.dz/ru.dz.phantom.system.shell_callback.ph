/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Callback for test purposes.
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
	var fio : .internal.connection;

    void run(var time : int ) [17]
    {
        console.putws("\n\nTimer callback: ");
        console.putws(time.toString());
        console.putws(" !!\n\n");

		// test of fio connection
		fio = new .internal.connection();
        fio.connect("fio:/amnt1/fio_log.txt");

        var data : .internal.object;

		data = fio.block(10, 0); // 1st arg is max read len

        console.putws("\nfio data: '");
        console.putws( data.toString() );
        console.putws("'\n");

		fio.block("written from phantom code", 1);

    }
                      
    void init(var tt : .internal.io.tty ) 
    {
        console = tt;
    }


};
