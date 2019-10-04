import .internal.io.tty;
import .internal.bitmap;
import .internal.string;
import .ru.dz.phantom.backgrounds;

package .ru.dz.windows;

class base
{
    var console : .internal.io.tty;
    var boot_object;

    void init( 	var _console : .internal.io.tty, var _booto )
    {
        console = _console;
        boot_object = _booto;
        console.putws(" in root...");

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




