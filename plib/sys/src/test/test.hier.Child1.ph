package .test.hier;

import .test.hier.Intermediate;

class Child1 extends Intermediate {
	var child1_init : int;

	void Child1()
	{
		child1_init = 1;
	}

	void test()
	{
		if( root_init != 1 ) throw "root init failed";
		if( child1_init != 1 ) throw "root init failed";
	}

};
