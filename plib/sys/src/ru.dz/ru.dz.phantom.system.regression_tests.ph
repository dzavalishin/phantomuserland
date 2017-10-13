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

import .internal.directory;
import .internal.double;
import .internal.float;
import .internal.long;
import .phantom.util.map;

class regression_tests
{
    var boot_object;
    var boot_success : int;
    var i : int;
    var j : int;
    var int_array : int [];
    var str_array : .internal.string [];
    var ctor_called : int;

	void regression_tests()
	{
		ctor_called = 3456;
	}


    void run (var _boot_object @const )
    {
        boot_object = _boot_object;
        boot_success = 0;

        print("Phantom language regression tests are running\n");

        flow_test();
        math_test();
        array_test();
        hashmap_directory_test();

        print("ctor_called ="); print(ctor_called); print("\n");
		if( ctor_called != 3456 )
			throw "constructor failed";

        long_test();
        float_test();
        double_test();


	// test c'tor call

		var map : .phantom.util.map;	
		map = new .phantom.util.map();

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

        //test_array = new void[]();
        int_array = new int[]();
        str_array = new .internal.string[]();

        str_array[0] = "zero";
        str_array[2] = "two";
        str_array[1] = "one";

        print("Beeping: ");
        i = 3;
        while( i )
        {
            i = i - 1;
            print("beep "); print(str_array[i]); print("! ");
        }
        print("\n");


        int_array[0] = 0;
        int_array[2] = 2;
        int_array[1] = 1;

        i = 3;
        while( i )
        {
            i = i - 1;
            if( int_array[i] != i )
            {
                print("array error: array["); print(i);
                print("] == "); print(int_array[i]);
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

        if( 
	(
		12
		/
		2
	) 
		!= 
	6 
	) 	throw "int error";

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
        print("Checking .internal.directory class operations... ");

        var dir : .internal.directory;
        var val : .internal.object;

        print("new\n");
        dir = new .internal.directory();

        //print("put 1\n");
        dir.put( "Hello", "world" );
        //print("put 2\n");
        dir.put( "Privet", "mir" );
        //print("put 3\n");
        dir.put( "Hola", "mundo" );

        //print("get 2\n");
        val = dir.get( "Privet" );
        if( !val.equals("mir") ) throw "dir error 1";

        //print("get 3\n");
        val = dir.get( "Hola" );
        if( !val.equals("mundo") ) throw "dir error 2";

        //print("get 1\n");
        val = dir.get( "Hello" );
        if( !val.equals("world") ) throw "dir error 3";

// put double
// check size
// delete
// check size
// check not exist
// put 4000 random ones, save copies
// check get
// delete some 2000
// put new 4000 random ones, save copies
// check get

        print("passed\n");
    }


    void long_test()
    {
        var a : .internal.long;
        var b : .internal.long;
        var c : .internal.long;

        print("start long tests\n");
/*
        a = (long)12;
        b = (long)2;
        c = (long)0-3;
*/
        a = 12;
        b = 2;
        c = 0-3;
        print("long tests 1\n");

        if( (a/b) != 6 ) 	throw "long error 1";
        if( (b-c) != 5 ) 	throw "long error 2";
        if( (b*c) != 0-6 ) 	throw "long error 3";
        if( (a+c) != 9 ) 	throw "long error 4";
        print("long tests 2\n");

        if( b<c ) 		throw "long error 5";
        if( b>a ) 		throw "long error 6";
        if( b<=c ) 		throw "long error 7";
        if( b>=a ) 		throw "long error 8";
        print("long tests passed\n");
    }


    void float_test()
    {
        var a : .internal.float;
        var b : .internal.float;
        var c : .internal.float;

        a = 12;
        b = 2;
        c = 0-3;

        if( (a/b) != 6 ) 	throw "float error 1";
        if( (b-c) != 5 ) 	throw "float error 2";
        if( (b*c) != 0-6 ) 	throw "float error 3";
        if( (a+c) != 9 ) 	throw "float error 4";

        if( b<c ) 		throw "float error 5";
        if( b>a ) 		throw "float error 6";
        if( b<=c ) 		throw "float error 7";
        if( b>=a ) 		throw "float error 8";
        print("float tests passed\n");
    }


    void double_test()
    {
        var a : .internal.double;
        var b : .internal.double;
        var c : .internal.double;

        a = 12;
        b = 2;
        c = 0-3;

        if( (a/b) != 6 ) 	throw "double error 1";
        if( (b-c) != 5 ) 	throw "double error 2";
        if( (b*c) != 0-6 ) 	throw "double error 3";
        if( (a+c) != 9 ) 	throw "double error 4";

        if( b<c ) 		throw "double error 5";
        if( b>a ) 		throw "double error 6";
        if( b<=c ) 		throw "double error 7";
        if( b>=a ) 		throw "double error 8";
        print("double tests passed\n");
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
