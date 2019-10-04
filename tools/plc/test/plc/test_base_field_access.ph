package .ru.dz.test;

// it is impossible to use untyped var in numeric expr, but possible as object.

class base
{
    var s : .internal.string;
    
    // If we are in same source compier fails to generate def c'tor
    void base() {}
};


class child extends base
{
	var s1;
	
	void method()
	{
	s = "Hello";
	s1 = s;
	}
};