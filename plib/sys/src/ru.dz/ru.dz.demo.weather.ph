/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Simple weater widget
 *
 *
**/

package .ru.dz.demo;

import .phantom.os;
import .internal.io.tty;
import .internal.window;
import .internal.bitmap;
import .internal.connection;
import .ru.dz.phantom.system.runnable;

attribute const * ->!;


class weather
{
    var i : int;
    var wbmp : .internal.bitmap[];

    var win : .internal.window;
    var sleep : .internal.connection;
    var curl : .internal.connection;

	var temperature : .internal.string;

    //var fio : .internal.connection;

    //var stat_conn : .internal.connection;


    void run(var console : .internal.io.tty)
    {
        var bmp : .internal.bitmap;

		console.putws("Weather win: init\n");

        bmp = new .internal.bitmap();
        bmp.loadFromString(getBackgroundImage());

        win = new .internal.window();

        win.setWinPosition(650,500);
        win.setTitle("Weather");
        win.setFg(0xFF000000); // black
		win.setBg(0xFFcce6ff); // light cyan

        win.clear();
		//win.drawImage( 0, 0, getBackgroundImage() ); // crashes kernel!
		win.drawImage( 0, 0, bmp );
		//win.drawString( 250, 250, "T -25 C" );
		win.drawString( 250, 250, "T =" );

		//bmp.paintTo( win, 0, 0 );
        win.update();

		console.putws("Weather win: done init\n");

		sleep = new .internal.connection();
        sleep.connect("tmr:");

		curl = new .internal.connection();
		curl.connect("url:http://smart.:8080/rest/items/Current_Outdoor_Air_Temp/state");

		temperature = curl.block(null, 0);
		console.putws("Weather win: curl = ");
		console.putws(temperature);
		console.putws("\n");
		win.drawString( 250, 280, temperature );


        while(1)
        {
			temperature = curl.block(null, 0);
			console.putws("Weather win: curl = ");
			console.putws(temperature);
			console.putws("\n");

			win.drawImage( 0, 0, bmp );
			win.drawString( 250, 280, temperature );
			win.drawString( 250, 250, "T =" );

			console.putws("Weather win: sleep\n");

            sleep.block(null, 10000);

			console.putws("Weather win: repaint\n");

		}



/* kills phantom with "Panic: Thread object has no INTERNAL flag" on snap load
		loadImages();

		console.putws("Weather win: got images\n");

        i = 0;

        sleep = new .internal.connection();
        sleep.connect("tmr:");

        //stat_conn = new .internal.connection();
        //stat_conn.connect("stt:");

        while(1)
        {
			console.putws("Weather win: sleep\n");

            sleep.block(null, 500);

			console.putws("Weather win: repaint\n");
            
			win.drawImage( 0, 0, wbmp[i] );            
			win.drawString( 250, 250, "T -26 C" );

            win.update();

			i = i + 1;
//			if( i >= 8 ) i = 0;
			if( i >= 3 ) i = 0;

        }
*/

    }

	.internal.string getTemperature()
	{
		//var url : .internal.string;

		//url = "http://smart.:8080/rest/items/Current_Outdoor_Air_Temp/state";

	}


    .internal.string getBackgroundImage()
    {
        return import "../resources/backgrounds/snow_weather.ppm" ;
    }


	.internal.bitmap loadImages()
	{

        wbmp[0] = new .internal.bitmap();
        wbmp[0].loadFromString(import "../resources/backgrounds/weather_clouds.ppm");

        wbmp[1] = new .internal.bitmap();
        wbmp[1].loadFromString(import "../resources/backgrounds/weather_grad.ppm");

        wbmp[2] = new .internal.bitmap();
        wbmp[2].loadFromString(import "../resources/backgrounds/weather_ice.ppm");
/*
        wbmp[3] = new .internal.bitmap();
        wbmp[3].loadFromString(import "../resources/backgrounds/weather_lightning.ppm");

        wbmp[4] = new .internal.bitmap();
        wbmp[4].loadFromString(import "../resources/backgrounds/weather_moon.ppm");

        wbmp[5] = new .internal.bitmap();
        wbmp[5].loadFromString(import "../resources/backgrounds/weather_rain.ppm");

        wbmp[6] = new .internal.bitmap();
        wbmp[6].loadFromString(import "../resources/backgrounds/weather_snow.ppm");

        wbmp[7] = new .internal.bitmap();
        wbmp[7].loadFromString(import "../resources/backgrounds/weather_sun.ppm");

        wbmp[8] = new .internal.bitmap();
        wbmp[8].loadFromString(import "../resources/backgrounds/weather_wind.ppm");
*/

	}
	
};

