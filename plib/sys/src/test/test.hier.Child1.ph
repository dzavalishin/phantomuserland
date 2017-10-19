package .test.hier;

import .test.hier.intermediate;

class child1 extends intermediate {
	var child1_init : int;

	void child1()
	{
		child1_init = 1;
	}

	void test()
	{
		if( root_init != 1 ) throw "root init failed";
		if( child1_init != 1 ) throw "root init failed";
	}

};
