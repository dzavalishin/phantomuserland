/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal: No
 * Preliminary: Yes
 *
 *
 **/

package .ru.dz.phantom.system;

/**
 *
 * Some compiler/VM regression tests.
 *
 **/


attribute const * ->!;


class regression_tests
{
    var boot_object;
    var boot_success : int;
    var i : int;
    var j : int;
    var test_array : void [];



    void run (var _boot_object @const )
    {
        boot_object = _boot_object;
        boot_success = 0;

        print("Phantom language regression tests are running\n");

        flow_test();
        math_test();
        array_test();
        hashmap_directory_test();
    }

    // ---------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------

    void print( var input : string )
    {
        boot_object.16(input);
    }


    // ---------------------------------------------------------------------
    // test arrays
    // ---------------------------------------------------------------------

    void array_test()
    {
        print("Checking arrays... ");

        test_array = new void[]();

        test_array[0] = "zero";
        test_array[2] = "two";
        test_array[1] = "one";

        print("Beeping: ");
        i = 3;
        while( i )
        {
            i = i - 1;
            print("beep "); print(test_array[i]); print("! ");
        }
        print("\n");


        test_array[0] = 0;
        test_array[2] = 2;
        test_array[1] = 1;

        i = 3;
        while( i )
        {
            i = i - 1;
            if( test_array[i] != i )
            {
                print("array error: array["); print(i);
                print("] == "); print(test_array[i]);
                throw "array error";
            }
        }


        print("passed\n");
    }

    // ---------------------------------------------------------------------
    // test basic math
    // ---------------------------------------------------------------------

    void math_test()
    {
        print("Checking int math... ");

        if( 2+2 != 4 ) throw "int + math error";
        if( 2-2 != 0 ) throw "int - math error";
        if( 2/2 != 1 ) throw "int / math error";
        if( 3*3 != 9 ) throw "int * math error";

        if( 3 < 2 ) throw "int < error";
        if( 5 > 6 ) throw "int > error";

        if( 3 <= 2 ) throw "int <= error";
        if( 5 >= 6 ) throw "int >= error";

        if( !(3 <= 3) ) throw "int <= error";
        if( !(5 >= 5) ) throw "int >= error";

        if( 0 || 0 )   throw "int || math error";
        if( 1 || 0 ) {} else  throw "int || math error";
        if( 0 || 1 ) {} else  throw "int || math error";
        if( 1 || 1 ) {} else  throw "int || math error";

        if( 0 && 0 )   throw "int && math error";
        if( 1 && 0 )   throw "int && math error";
        if( 0 && 1 )   throw "int && math error";
        if( 1 && 1 ) {} else  throw "int && math error";

        if( 0 | 0 )   throw "int | math error";
        if( 1 | 0 ) {} else  throw "int | math error";
        if( 0 | 1 ) {} else  throw "int | math error";
        if( 1 | 1 ) {} else  throw "int | math error";

        if( 0 & 0 )   throw "int & math error";
        if( 1 & 0 )   throw "int & math error";
        if( 0 & 1 )   throw "int & math error";
        if( 1 & 1 ) {} else  throw "int & math error";

        print("passed\n");
    }

    // ---------------------------------------------------------------------
    // test flow control
    // ---------------------------------------------------------------------

    void test_do_while()
    {
        // test do/while
        i = 3;
        j = 0;
        do { j = j + 1; } while(i) i = i - 1;

        if( j != 4 ) throw "do/while error";
    }

    void flow_test()
    {
        print("Checking flow control operators... ");

        if( 0 ) throw "if(0) error";
        if( 0 == 1 ) throw "if or == error";
        if( 0 != 0 ) throw "if or != error";

        // try to check that if never execs 'true' condition operator
        //print("try to check that if never execs 'true' condition operator... ");

        j = 0;
        if( 1 ) j = 1;
        if( j == 1 ) {} // empty
        else
            throw "if(1) error";

        test_do_while();

        i = 2;
        switch(i)
        {
        case 1:  throw "switch fall in error";
        case 2:  break;
        case 3:  throw "switch fall out error";
        default: throw "switch default error";
        }

        print("passed\n");
    }


    void hashmap_directory_test()
    {
        print("Checking .internal.directory class operators... ");

        var dir : new .internal.directory;
        var val;

        dir = new .internal.directory();

        dir.put( "Hello", "world" );
        dir.put( "Privet", "mir" );
        dir.put( "Hola", "mundo" );

        var = dir.get( "Privet" );
        if( !var.equals("mir") ) throw "dir error 1";

        var = dir.get( "Hola" );
        if( !var.equals("mundo") ) throw "dir error 2";

        var = dir.get( "Hello" );
        if( !var.equals("world") ) throw "dir error 3";


        print("passed\n");
    }

};





// Historical junk.
//
// $Log: compiler_regression_test.ph,v $
// Revision 1.6  2005/02/27 21:14:17  dz
// Cleanup
//
// Revision 1.5  2005/02/27 11:54:45  dz
// import, etc
//
// Revision 1.4  2005/02/24 06:42:40  dz
// plc is working
//
// Revision 1.3  2005/02/24 05:46:45  dz
// *** empty log message ***
//
//
