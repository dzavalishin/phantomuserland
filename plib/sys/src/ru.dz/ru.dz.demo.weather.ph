/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
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
import .internal.tcp;
import .internal.time;
import .internal.directory;
import .internal.long;
//import .ru.dz.phantom.system.runnable;

attribute const * ->!;

// graph color: 180 R, 205 G, 147 B
// ABGR hex = 0xFF93CDB4

class weather
{
    var i : int;
    var wbmp : .internal.bitmap[];

    var win : .internal.window;
    var sleep : .internal.time;
    //var curl : .internal.connection;
    var http : .internal.tcp;

	//var temperature : .internal.string;
	var itemp : .internal.long;

	var xpos : int;
	var ypos : int;

    void run(var console : .internal.io.tty)
    {
        var bmp : .internal.bitmap;
        var bmpw : .internal.bitmap;
        var json_string : .internal.string;
        var json : .internal.directory;
        var jtmp : .internal.directory;

		//console.putws("Weather win: init\n");

        bmp = new .internal.bitmap();
        bmp.loadFromString(getBackgroundImage());

        win = new .internal.window();

        win.setWinPosition(650,500);
        //win.setWinSize( 374, 268 );

        win.setTitle("Weather");
        //win.setFg(0xFF000000); // black
		//win.setBg(0xFFcce6ff); // light cyan

        win.setFg(0xFF93CDB4); // light green
		win.setBg(0xFFccccb4); // light milk

        win.clear();
		win.drawImage( 0, 0, bmp );
		//win.drawString( 250, 250, "T =" );

        //loadImages();
        bmpw = new .internal.bitmap();
        bmpw.loadFromString(import "../resources/backgrounds/weather_sun_sm.ppm");
        win.drawImage( 17, 102, bmpw );

        win.update();

		//console.putws("Weather win: done init\n");

		sleep = new .internal.time();

        http = new .internal.tcp();

		/*
		temperature = curl.block(null, 0);
		console.putws("Weather win: curl = ");
		console.putws(temperature);
		console.putws("\n");
		win.drawString( 280, 240, temperature );
		*/

        xpos = 17;

        while(1)
        {
			console.putws("Weather win: curl\n");
            //temperature = curl.block(null, 0);

            json_string = http.curl( "http://api.weather.yandex.ru/v1/forecast?extra=true&limit=1", "X-Yandex-API-Key: 7bdab0b4-2d21-4a51-9def-27793258d55d\r\n" );

            //json = (.internal.directory)json_string.parseJson();
            json = json_string.parseJson();
            jtmp = json.get("fact");
            //temperature = jtmp.get("temp").toString();
            itemp = jtmp.get("temp");
                
			console.putws("Weather win: curl = ");
            //console.putws(temperature);
            console.putws(itemp.toString());
			console.putws("\n");

			//win.drawImage( 0, 0, bmp );
            //win.drawImage( 17, 102, bmpw );
            win.drawImagePart( 0, 0, bmp, 250, 240, 120, 22 );


            win.setFg(0); // black
			win.drawString( 280, 235, itemp.toString() );
			win.drawString( 250, 235, "T =" );

            //itemp = temperature.toInt();
            ypos = 15 + (itemp * 2);

            win.setFg(0xFF93CDB4); // light green
            if( ypos < 70 )
                win.fillBox( xpos, ypos, 2, 2 );

			console.putws("Weather win: sleep\n");

            sleep.sleepSec( 60 );

            xpos = xpos+1;
            if( xpos > 358 ) xpos = 17;
		}



/* kills phantom with "Panic: Thread object has no INTERNAL flag" on snap load
		loadImages();

		console.putws("Weather win: got images\n");

        i = 0;

        sleep = new .internal.time();

        //stat_conn = new .internal.connection();
        //stat_conn.connect("stt:");

		win.setAutoUpdate( 0 );

        while(1)
        {
			console.putws("Weather win: sleep\n");

            sleep.sleepMsec( 500 );

			console.putws("Weather win: repaint\n");
            
			win.drawImage( 0, 0, wbmp[i] );            
			win.drawString( 250, 240, "T -26 C" );

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
        return import "../resources/backgrounds/weather_window.ppm" ;
        //return import "../resources/backgrounds/snow_weather.ppm" ;
    }


	.internal.bitmap loadImages()
	{
        wbmp[7] = new .internal.bitmap();
        wbmp[7].loadFromString(import "../resources/backgrounds/weather_sun_sm.ppm");

/*
        wbmp[0] = new .internal.bitmap();
        wbmp[0].loadFromString(import "../resources/backgrounds/weather_clouds.ppm");

        wbmp[1] = new .internal.bitmap();
        wbmp[1].loadFromString(import "../resources/backgrounds/weather_grad.ppm");

        wbmp[2] = new .internal.bitmap();
        wbmp[2].loadFromString(import "../resources/backgrounds/weather_ice.ppm");

        wbmp[3] = new .internal.bitmap();
        wbmp[3].loadFromString(import "../resources/backgrounds/weather_lightning.ppm");

        wbmp[4] = new .internal.bitmap();
        wbmp[4].loadFromString(import "../resources/backgrounds/weather_moon.ppm");

        wbmp[5] = new .internal.bitmap();
        wbmp[5].loadFromString(import "../resources/backgrounds/weather_rain.ppm");

        wbmp[6] = new .internal.bitmap();
        wbmp[6].loadFromString(import "../resources/backgrounds/weather_snow.ppm");


        wbmp[8] = new .internal.bitmap();
        wbmp[8].loadFromString(import "../resources/backgrounds/weather_wind.ppm");
*/

	}
	
};

