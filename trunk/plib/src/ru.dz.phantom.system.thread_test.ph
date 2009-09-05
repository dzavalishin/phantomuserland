/**
 *
 * Test code used to check snapshots.
 *
**/


package .ru.dz.phantom.system;

import .internal.io.tty;

attribute const * ->!;


class thread_test // extends runnable
{
    var console : .internal.io.tty;
    var incr :  int;

	
	
    // They call us here
    void run(var parent_object @const ) [8] // don't have to give number here? Must be inherited?
    {
        console = new .internal.io.tty();
        incr = 1;

        while(1)
        {
            console.putws("Thread is running... ");
            console.putws("... ");
            incr = incr + 1;
            console.putws(incr.5());
            console.putws("  ");
        }
    }

};


