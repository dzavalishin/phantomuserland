/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Callback for test purposes.
 *
**/

package .ru.dz.phantom.system;

import .phantom.os;
import .internal.io.tty;
import .internal.window;
import .internal.time;
import .internal.io;
import .phantom.runnable;

attribute const * ->!;


class shell_callback //extends runnable
{
    var console : .internal.io.tty;
    var fio : .internal.io;

    void run(var time : int ) [17]
    {
        console.putws("\n\nTimer callback: ");
        console.putws(time.toString());
        console.putws(" !!\n\n");

        // test of fio connection
        fio = new .internal.io();
        fio.open("fio:/amnt1/fio_log.txt");

        var data : .internal.object;

        data = fio.read(10); // 1st arg is max read len

        console.putws("\nfio data: '");
        console.putws( data.toString() );
        console.putws("'\n");

        fio.write("written from phantom code");

        fio.close();
    }
                      
    void init(var tt : .internal.io.tty ) 
    {
        console = tt;
    }


};
