import .internal.io.tty;
import .internal.bitmap;
import .internal.string;
import .ru.dz.phantom.backgrounds;

package .ru.dz.windows;

class base
{
    var console : .internal.io.tty;
    var boot_object;
    //var bmp : .internal.bitmap;
    //var bkg : .ru.dz.phantom.backgrounds;

    void init( 	var _console : .internal.io.tty, var _booto )
    {
        console = _console;
        boot_object = _booto;
        console.putws(" in root...");

        //bkg = new .ru.dz.phantom.backgrounds();
        //bmp = new .internal.bitmap();

		/*
        console.putws("\nwill load bitmap...\n");
        var bmstring : string;
        bmstring = bkg.getBackgroundImage();
        bmp.loadFromString(bmstring);
        console.putws("have bitmap!...\n");
        */
    }

    void gotoxy( var x : int, var y : int)
    {
        console.gotoxy(x,y);
    }

    void putws( var s : string )
    {
        console.putws(s);
    }



};




