/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 *
 * Shell - class that is started from boot env to let user run apps. Now just runs some test code.
 *
 *
**/

package .ru.dz.phantom.system;

import .phantom.os;
import .internal.io.tty;
import .internal.window;
//import .internal.mutex;
import .internal.time;
import .internal.stat;

import .phantom.runnable;
import .ru.dz.phantom.system.shell_callback;

import .test.test_mutex.test_mutex;

//import .test.suite;

import .ru.dz.demo.start;

attribute const * ->!;


//class shell
class shell extends .phantom.runnable
{
    var console : .internal.io.tty;
    var incr :  int;
    var stat_val :  int;
    var stat_pos :  int;
    var stat_next_pos :  int;

    var cpu_idle : int;
    var old_idle : int;

    var white : int;

    var win : .internal.window;
    var timer : .internal.time;


    var stat : .internal.stat;

    var cb : shell_callback;

    //var mtx : .internal.mutex;

    var demo : .ru.dz.demo.start;
	var test_app : .test.test_mutex.test_mutex;

    void run(var parent_object @const ) [8]
    {

        win = new .internal.window();
        win.setWinPosition(50,310);
        win.setTitle("Disk io stats");
        win.setFg(0xFF000000); // black
        //win.setBg(0xFFFFFFFF); // white
        win.clear();
        //win.drawBox( 10, 10, 20, 20 );
        //win.drawString( 22, 22, "Say Hello to Win!" );
        win.update();
        stat_pos = 0;
        old_idle = 0;

        white = 0xFFFFFFFF;

        console = new .internal.io.tty();
        incr = 1;

        console.moveWindow(10,10);
        console.setTitle("VM Shell");
        console.setbgcolor( 0xFFFFFFFF );
        console.clear();

        timer = new .internal.time();

        // test of connections - TODO test new timer callback
        //conn = new .internal.time();

        //cb = new shell_callback();
        //cb.init( console );

        //conn.runLater( cb, 20000 );


        stat = new .internal.stat();

		test_app = new .test.test_mutex.test_mutex();
		//test_app.start(); // temp off to debug VNC

        demo = new .ru.dz.demo.start();
        demo.run(console);

        while(1)
        {
            //mtx.lock();

            console.putws("I am connected shell!\n");
            console.putws("--------------------------------------------------\n");
            console.putws("Thread is running... ");
            console.putws(incr.toString());
            console.putws("  ");

            timer.sleepMsec( 500 );

            stat_val = stat.getStat( 26, 0 ); // blk io per sec
            console.putws("blk io =. ");

            cpu_idle = stat.getIdle(); // cpu 0 idle
            console.putws("cpu idle = ");
            console.putws(cpu_idle.toString());
            console.putws("\n");

            cpu_idle = 100 - cpu_idle;

            stat_next_pos = stat_pos + 1;
            if( stat_next_pos >= win.getXSize() )
                stat_next_pos = 0;

            // Bar
            win.setFg( 0xFF00FF00 ); // Green
            win.drawLine( stat_next_pos, 5, 0, 300 );

            // Clear
            win.setFg( white ); // white
            win.drawLine( stat_pos, 5, 0, 300 );

            win.setFg(0xFF000000); // black
            win.drawLine( stat_pos, 5, 0, 0+stat_val );

            // Idle
            win.setFg( 0xFF0000FF ); // Blue
            win.drawLine( stat_pos-1, old_idle, 1, cpu_idle-old_idle );

            old_idle = cpu_idle;
            stat_pos = stat_next_pos;

            /*
             if( stat_pos >= win.getXSize()-1 )
             win.scrollHor( 0, 0, win.getXSize(), win.getYSize(), 0-1 );
             else
             stat_pos = stat_pos + 1;
             */

            win.update();

            //mtx.unlock();

            incr = incr + 1;
        }


    }
};

