package .test.test_mutex;


import .phantom.runnable;
import .internal.mutex;
import .internal.world;

class sub extends .phantom.runnable
{
	var m : .internal.mutex;
	var w : .internal.world;
	var s : .internal.string;

	void init( var mu : .internal.mutex, var wo : .internal.world, var st : .internal.string )
	{
		m = mu;
		w = wo;
		s = st;
	}

    void run( var arg )
    {
		while(1)
		{
			m.lock();
			w.log( "\n" );
			w.log( s );
			w.log(": message 1 ..");
			w.log("message 2 ..");
			w.log("message 3 ..");
			w.log("message 4 ..");
			w.log("message last\n");
			m.unlock();
		}
    }


};
