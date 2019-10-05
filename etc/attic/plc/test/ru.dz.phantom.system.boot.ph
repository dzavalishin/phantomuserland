

package .ru.dz.phantom.system;

import .ru.dz.phantom.system.regression_tests;
import .ru.dz.phantom.system.class_loader;

import .internal.io.tty;
import .internal.binary;
//import .ru.dz.windows.root;
//import .ru.dz.phantom.system.thread_test;

import .ru.dz.phantom.backgrounds;
import .internal.bitmap;

// This is the first class ever loaded by Phantom.
// 8-th method of it will be called and some special
// boot object will be passed in.

// It is worth top mention that this code runs just ONCE in the
// any Phantom instance's LIFE. At the very first OS instance start.
// At the next start OS continues work from the point it was stopped at.

// In fact, real OS work begins after the startup() method is finished.
// In real kernel this method runs a thread (or threads), and finishes.
// Those threads are not run until startup() is over. Just after the
// finish of startup() method OS will continue as if it was snapshotted
// with all the changes made with startup() and with all the threads
// startup() started.

// Current Windows-based Phantom emulator is not able to run threads,
// so it just runs startup() and exits after. You have to call all
// the code you want to test from startup() below.


// TODO: Compiler has to have some way of loading attributes
// from class files!
attribute const * ->!; 


class boot
{
    var debug : int;
    var boot_object;

    var loader_class : class_loader;
    var loader : class_loader;

    var reg_test_class;
    var reg_test : .ru.dz.phantom.system.regression_tests;

    var console : .internal.io.tty;

    var windows : .ru.dz.windows.root;

    var run : .ru.dz.phantom.system.thread_test;

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

        // compiler regression tests

        reg_test = new .ru.dz.phantom.system.regression_tests();
        print("Starting compiler regression tests\n");
        reg_test.run(boot_object);
        print("Finished compiler regression tests\n");

        // TODO: This is temporary and is working only on first boot
        setScreenBackgroud();

        console = new .internal.io.tty();

        console.putws("Creating root window...\n");

        windows = new .ru.dz.windows.root();
        console.putws(" initing root window...\n");
        windows.init(console,boot_object);

        console.putws(" starting windows test...\n");
        windows.test();

        print("Finished windows tests\n");

        run = new .ru.dz.phantom.system.thread_test();

        boot_object.18(run);
        //run.8(this);

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





/**
 *
 * Historical artefact...
 *
 * $Log: boot.ph,v $
 * Revision 1.6  2005/03/07 20:37:53  dz
 * text console
 *
 * Revision 1.5  2005/03/01 11:58:25  dz
 * Automatic variables done
 *
 * Revision 1.4  2005/03/01 11:11:45  dz
 * Type checks started, scalar assignment is mostly done
 *
 * Revision 1.3  2005/02/28 19:56:24  dz
 * Error reporting fixes
 *
 * Revision 1.2  2005/02/28 19:44:24  dz
 * Inheritance is basically done. No overloading, no checks are done for
 * []-style ordinal settings.
 *
 * Revision 1.1  2005/02/28 06:24:45  dz
 * Bootloader
 *
 *
**/


