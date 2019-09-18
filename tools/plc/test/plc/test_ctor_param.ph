package .ru.dz.test;



class withparam
{
	var s1 : .internal.string;
	
    void withparam( var s : .internal.string )
    {
    	s1 = s;
    }
};


class caller
{
	var o : withparam;
	
	void method()
	{
		o = new withparam( "Hello" );
		//o = new withparam( );
	}
};