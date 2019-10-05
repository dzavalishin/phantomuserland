import .internal.io.tty;
import .internal.bitmap;
import .internal.string;
//import .ru.dz.windows.window;
import .ru.dz.phantom.backgrounds;

package .ru.dz.windows;

class root // extends base
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


/*
    window window()
    {
        var ret : .ru.dz.windows.window;
        ret = new .ru.dz.windows.window();
        ret.init(this);

        ret.display();

        return ret;
    }
*/
    void test()
    {
        console.gotoxy(0,3);
        console.putws("Hello on new console (root win)!");
        console.gotoxy(10,10);
        console.setcolor(14);
        console.putws("Pos 10,10");


        console.gotoxy(0,0);
        console.putws("\n");

        //bmp.paintTo( console, 0, 0 );

	/*
        while(1)
        {

            try {
                shell();
            }
            catch( string o ) {
                console.putws("exception: ");
                console.putws(o);
                console.putws("\n");
            }

        }*/

        //var w : .ru.dz.windows.window;

        //	w = window();
        //console.putws(" ...after window init");
        //	w.display();
        //console.putws(" ...after window.display");
    }

    //        var ch : string;
    void shell()
    {
        //        var ch : string;
        //.internal.string ch;
        var ch : .internal.string;
        var str :  .internal.string;
        console.putws("\nThe very first Phantom command line shell\n");

        str = "";

        while(1)
        {
            ch = console.getwc();
            //console.putws("got char\n");

            if( ch.equals("\n") )
            {
                if(str.equals("quit")) break;
                putws("\n");
                shell_cmd(str);
                str = "";
                console.putws("\nphantom => \n");
                //break;
            }
            else
                str = str.concat( ch );

            //console.putws(ch.getClass());

            //console.debug(ch);
            console.putws(ch);
        }
        throw "shell quit";

    }


    void shell_cmd( var cmd : string)
    {
        var i : int;
        var c : int;
        var spacePos : int;
        var len : int;

        //console.debug(cmd);

        len = cmd.length();

        //putws("Cmd is '"); putws(cmd); putws("'\n");

        i = 0;
        spacePos = 0;

        while( i < len )
        {
            c = cmd.charAt(i);
            if( c == 32 )
            {
                spacePos = i;
                break;
            }
            i = i+1;
        }


        var verb : string;
        var parms : string;

        if( i >= len )
        {
            verb  = cmd;
            parms = "";
        }
        else
        {
            verb  = cmd.substring( 0, spacePos );
            parms = cmd.substring( spacePos+1, len - spacePos );
        }


        if( verb.equals("ls") )
        {
            putws("We're not in Linux anymore...\n");
            return;
        }

        if( verb.equals("man") )
        {
            putws("Politically correct version of this command is 'person' now\n");
            return;
        }

        if( verb.equals("cd") )
        {
            putws("Nowhere to cd yet...\n");
            return;
        }

        if( verb.equals("echo") )
        {
            putws(parms);
            return;
        }

        if( verb.equals("ps") )
        {
            putws(parms);
            return;
        }

        if( verb.equals("exit") )
        {
            throw parms;
            return;
        }

        if( verb.equals("help") )
        {
            putws("help, echo, exit - that's all I know yet...");
            return;
        }

        putws(("Unknown command: ").concat(verb));


    }

};




