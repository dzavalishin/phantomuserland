package .ru.dz.phantom.tetris;

import .ru.dz.phantom.system.class_loader;

import .internal.io.tty;
import .internal.binary;

import .ru.dz.phantom.backgrounds;
import .internal.bitmap;

/*
** Tetris game - replacement boot stub to call tetris
*/

import .ru.dz.phantom.tetris.imagesmanager;
import .ru.dz.phantom.tetris.mainmodule;

// This is the first class ever loaded by Phantom.
// 8-th method of it will be called and some special
// boot object will be passed in.

// This one is HACK and used to load Tetris as long as we don't have some better way
attribute const * ->!; 


class boot
{
    var debug : int;
    var boot_object;

    var loader_class : .ru.dz.phantom.system.class_loader;
    var loader : .ru.dz.phantom.system.class_loader;

    var reg_test_class;

    var console : .internal.io.tty;


    // They call us here
    void startup(var _boot_object @const ) [8]
    {
        try {
            do_startup( _boot_object );
        }
        catch( string e )
        {
            print("\r\nException: '"); print(e); print("'");
        }
    }

    void do_startup(var _boot_object @const )
    {
        debug = 1;
        boot_object = _boot_object;
        print("Phantom System Envirinment Setup is running\n");
        
        // Now setup class loader for the system
        
        if( debug ) print("loading loader class\n");
        loader_class = load_class(".ru.dz.phantom.system.class_loader");

        if( debug ) print("creating loader object\n");
        loader = new *(loader_class)();

        if( debug ) print("initializing loader\n");
        loader.init(boot_object);

        if( debug ) print("registering loader\n");
        boot_object.17(loader); // register new loader in the system

        // TODO: This is temporary and is working only on first boot
        setScreenBackgroud();

	while(1) tetris();
        
    }

    void tetris()
    {
        try {
        console = new .internal.io.tty();

		var tetrisMainModule : .ru.dz.phantom.tetris.mainmodule;
		tetrisMainModule = new .ru.dz.phantom.tetris.mainmodule();
		tetrisMainModule.init( boot_object, console, 10, 15 );
		tetrisMainModule.runGame( );
        print("Finished tetris.\n");
        } 
	catch( string e )
        {
            print("\r\nException: '"); print(e); print("'");
        }    
    }
    

    void setScreenBackgroud()
    {
        var bmp : .internal.bitmap;
        var bkg : .ru.dz.phantom.backgrounds;

        bkg = new .ru.dz.phantom.backgrounds();
        bmp = new .internal.bitmap();

        print("\nwill load bitmap...\n");
        var bmstring : string;
        bmstring = bkg.getBackgroundImage();
        bmp.loadFromString(bmstring);
        print("have bitmap!...\n");

        // Now set background!
        boot_object.20(bmp);

    }

    // ---------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------

    void print( var input : string )
    {
        boot_object.16(input);
    }

    .internal.object load_class( var name : string )
    {
        return boot_object.8(name);
    }

    .internal.binary newBinary( var nbytes : int )
    {
        return boot_object.19(nbytes);
    }



};




