package .test;

import .internal.io.tty;
import .test.hier.Child1;

class suite {
    var console : .internal.io.tty;


	void suite()
	{
        console = new .internal.io.tty();
        console.clear();
        console.setTitle( "Regress tests" );
	}

	void print( var s : string )
	{
		console.putws(s);
	}

	void println( var s : string )
	{
		console.putws(s);
		console.putws("\n");
	}

	void run()
	{
		println("Run tests suit");

		println("Run hierarchy constructors test");
		var c1 : hier.Child1;
		c1 = new hier.Child1();
		c1.test();
		println("PASSED: hierarchy constructors test");
	}

};
