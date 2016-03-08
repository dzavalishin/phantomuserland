/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Simple demo
 *
 *
**/

package .ru.dz.demo;

import .phantom.os;
import .internal.io.tty;
//import .internal.window;
//import .internal.bitmap;
//import .internal.connection;
//import .ru.dz.phantom.system.runnable;

import .ru.dz.demo.weather;
import .ru.dz.demo.chart;

attribute const * ->!;


class start
{
	var wv : .ru.dz.demo.weather;
	var cv : .ru.dz.demo.chart;

    void run(var console : .internal.io.tty)
    {
		wv = new  .ru.dz.demo.weather();
		wv.run(console);

		//cv = new .ru.dz.demo.chart();

    }


	
};

