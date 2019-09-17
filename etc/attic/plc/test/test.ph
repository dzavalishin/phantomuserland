/**
 *
 * $Log: test.ph,v $
 * Revision 1.8  2005/02/28 15:04:57  dz
 * Base classes and interfaces implemented (not tested well)
 *
 *
**/


package .ru.dz.test;

//import .internal.*;
import .ru.dz.phantom.system.iterator;
import .ru.dz.phantom.system.regression_tests;
import .ru.dz.phantom.system.class_loader;


interface test_iface
{
	void interface_m( var v: string );
};

class parent
{
void test( var i: int ) {
	int avar;
	
	avar = i;
}
int i;
};

class child extends parent implements test_iface
{
void test( var i: int ) {}
var j;
	void interface_m( var v: string ) {}
};

