package .test.test_mutex;

import .internal.mutex;
import .internal.world;

import .phantom.application;

import .test.test_mutex.sub;


class test_mutex extends .phantom.application
{
	var sub1 : .test.test_mutex.sub;
	var sub2 : .test.test_mutex.sub;

	var m : .internal.mutex;
	var ww : .internal.world;

    void run( var arg )
    {
        ww = new .internal.world();

		ww.log("test_mutex started");

		m = new .internal.mutex();

		// crashes compiler
		//sub1 = new sub( m, ww, "Sub1" );
		//sub2 = new sub( m, ww, "Sub2" );

		sub1 = new sub();
		sub2 = new sub();

		sub1.init( m, ww, "Sub1" );
		sub2.init( m, ww, "Sub2" );

		ww.startThread( sub1, 0 );
		ww.startThread( sub2, 0 );

		ww.log("started 2 threads, exit");
    }

	

};
