/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * User boot code.
 *
**/

package .ru.dz.phantom.system;

// This is the first class ever loaded by Phantom. An 8-th method of it
// will be called and some special boot object will be passed in.

// It is worth to mention that this code runs just ONCE in the
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


import .ru.dz.phantom.system.regression_tests;
import .ru.dz.phantom.system.class_loader;

import .internal.io.tty;
import .internal.binary;
import .ru.dz.windows.root;

import .ru.dz.phantom.system.thread_test;
import .ru.dz.phantom.system.shell;
import .ru.dz.phantom.system.runnable;

import .ru.dz.phantom.backgrounds;
import .internal.bitmap;
import .internal.window;
import .internal."class";

import .phantom.osimpl;

import .test.suite;

//import .test.toPhantom.Assigns;

/*
import .test.toPhantom.PhantomPrinter;
import .test.toPhantom.AllRun;
*/


// TODO: Compiler has to have some way of loading attributes
// from class files!
attribute const * ->!; 


class boot
{
    var debug : int;
    var boot_object : .internal.object;

    var loader_class : .internal."class"; // class_loader;
    var loader : class_loader;

    var reg_test_class;
    var reg_test : .ru.dz.phantom.system.regression_tests;

    var console : .internal.io.tty;

    var windows : .ru.dz.windows.root;

//    var run : .ru.dz.phantom.system.thread_test;
//    var run : .ru.dz.phantom.system.shell;
    var run : .ru.dz.phantom.system.runnable;

    var shell_name : string;
    var shell_class : .internal."class";

    // They call us here
    void startup(var _boot_object @const ) [8]
    {
        try {
//            do_startup( _boot_object );
            boot_object = _boot_object;
            do_startup();
        }
        catch( string e )
        {
            print("\r\nException: '"); print(e); print("'");
        }
    }


    //void do_startup(var _boot_object : .internal.object @const )
    void do_startup()
    {
// uncomment to run Java unit tests
/*
		var pp : .test.toPhantom.PhantomPrinter;
		var ar : .test.toPhantom.AllRun;
*/
        debug = 1;

        //boot_object = _boot_object;

        print("Phantom System Environment Setup is running\n");


        // Now setup class loader for the system

        if( debug ) print("loading loader class\n");
        loader_class = load_class(".ru.dz.phantom.system.class_loader");

        if( debug ) print("creating loader object\n");
        loader = new *(loader_class) : .ru.dz.phantom.system.class_loader ();

        if( debug ) print("initializing loader\n");
        loader.init(boot_object);

        if( debug ) print("registering loader\n");
        boot_object.17(loader); // register new loader in the system

        // compiler regression tests

        reg_test = new .ru.dz.phantom.system.regression_tests();
        print("Starting compiler regression tests\n");
        reg_test.run(boot_object);
        print("Finished compiler regression tests\n");

        runJavaTests();

        // TODO: This is temporary and is working only on first boot
        //setScreenBackgroud();

        console = new .internal.io.tty();

	// setup global OS interface object NOW
        var oi : .phantom.osimpl;
        oi = get_os_interface_object();
        // Pass it to OS to be made accessible from all threads
        boot_object.22( oi ); 

        shell_name = oi.getKernelEnvironmentValue("root.shell");

        print("Env root.shell=");
        print(shell_name);
        print("\n");
        print("Env root.init=");
        print(oi.getKernelEnvironmentValue("root.init"));
        print("\n");


        console.putws("\n\n\n\n\n\n\n\n\n\n\n\n\n\n");

        console.putws("Env root.shell=");
        console.putws(shell_name);
        console.putws("\n");
        console.putws("Env root.init=");
        console.putws(oi.getKernelEnvironmentValue("root.init"));
        console.putws("\n");

        console.putws("Creating root window...\n");

        windows = new .ru.dz.windows.root();
        console.putws(" initing root window...\n");
        windows.init(console,boot_object);

        console.putws(" starting windows test...\n");
        windows.test();

        print("Finished windows tests\n");


		// Run tests in plib/sys/src/test
		var suite : .test.suite;
		suite = new .test.suite();
		suite.run();

// uncomment to run Java unit tests
/*
        print("Run Java tests\n");
		pp = new .test.toPhantom.PhantomPrinter();
		pp.init( console );

		ar = new .test.toPhantom.AllRun();
		ar.setPrinter( pp );
		ar.runAll();
*/

        //run = new .ru.dz.phantom.system.thread_test();
        // TODO runtime class check!
        shell_class = load_class(shell_name);
        run = new *(shell_class)();

        //run.init();

        boot_object.18(run); // Thread run
        //boot_object.18(run); // 2nd thread in same object
//        run.go();

        //run.8(this);

        // TODO: This is temporary and is working only on first boot
        setScreenBackgroud();
    }


    void setScreenBackgroud()
    {
        var bmp : .internal.bitmap;
        var bkg : .ru.dz.phantom.backgrounds;

        bkg = new .ru.dz.phantom.backgrounds();
        bmp = new .internal.bitmap();

        print("----------------------\nwill load bitmap...\n");
        var bmstring : string;
        bmstring = bkg.getBackgroundImage();
        bmp.loadFromString(bmstring);
        print("have bitmap...\n");

        // Now set background!
        boot_object.20(bmp);
    }


    .phantom.osimpl get_os_interface_object()
    {
        var os : .phantom.osimpl;
        os = new .phantom.osimpl();

        os.init(boot_object);

        return os;
    }
    void runJavaTests()
    {
/*
        var rc : int;

        print("Running tests of Java compiler ...\n");

        var t1: .test.toPhantom.Assigns;
        t1 = new .test.toPhantom.Assigns();
        rc = t1.runTest();
        if( rc != 0 ) print("Java test FAILED: Assigns\n");
        else print("Java test PASSED: Assigns\n");

*/
    }

    // ---------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------

    void print( var input : string )
    {
        boot_object.16(input);
    }

    .internal."class" load_class( var name : string )
    {
        return boot_object.8(name);
    }

    .internal.binary newBinary( var nbytes : int )
    {
        return boot_object.19(nbytes);
    }

    void sleep()
    {
        // it will wait not less than for a second in win env anyway yet
        boot_object.21(1000);
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


