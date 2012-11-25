/**
 *
 * Phantom compiler regression test.
 * 
 * Extends and implements.
 *
**/


package .ru.dz.test;

//import .ru.dz.phantom.system.iterator;
//import .ru.dz.phantom.system.regression_tests;
//import .ru.dz.phantom.system.class_loader;


interface test_iface
{
    void interface_m( var v: string );
};

class parent
{
    var i : int;

    void test( var i: int ) 
    {
	var avar : int;
	
	avar = i;
    }

};

class child extends parent implements test_iface
{
    void test( var i: int ) {}
    void interface_m( var v: string ) {}

    var j;

};

