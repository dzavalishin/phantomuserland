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
import .internal.mutex;
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
    var stat_val :  int;
    var stat_pos :  int;

    var win : .internal.window;
    var conn : .internal.connection;

    var stat_conn : .internal.connection;

    var cb : shell_callback;

    var mtx : .internal.mutex;

    void init()
    {
        mtx = new .internal.mutex();
        mtx.lock();
    }

    void go()
    {
        mtx.unlock();
    }

    void run(var parent_object @const ) [8]
    {

        win = new .internal.window();
        win.setWinPosition(50,310);
        win.setTitle("Disk io stats");
        win.setFg(0); // black
        //win.setBg(0xFFFFFFFF); // white
        win.clear();
        //win.drawBox( 10, 10, 20, 20 );
        //win.drawString( 22, 22, "Say Hello to Win!" );
        win.update();
        stat_pos = 0;


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

        stat_conn = new .internal.connection();
        stat_conn.connect("stt:");

        while(1)
        {
            mtx.lock();

            console.putws("I am connected shell!\n");
            console.putws("--------------------------------------------------\n");
            console.putws("Thread is running... ");
            console.putws(incr.toString());
            console.putws("  ");

            conn.block(null, 500);

            stat_val = stat_conn.block( 26, 0 ); // blk io per sec
            console.putws("blk io =. ");
            console.putws(stat_val.toString());
            console.putws("\n");

            win.drawLine( stat_pos, 5, 0, 0+stat_val );

            
            stat_pos = stat_pos + 1;
            if( stat_pos >= win.getXSize() )
                stat_pos = 0;
            
            /*
            if( stat_pos >= win.getXSize()-1 )
	        win.scrollHor( 0, 0, win.getXSize(), win.getYSize(), 0-1 );
            else
                stat_pos = stat_pos + 1;
            */

            win.update();

            mtx.unlock();

            incr = incr + 1;
        }


    }
};

