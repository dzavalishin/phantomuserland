/**
 *
 * Phantom compiler regression test.
 * 
 * Import expression.
 *
**/

package .ru.dz.test;


class test_import
{

    string run_test()
    {
    var xmldata : string;
    
    xmldata = import "README";
    //xmldata = import "build.xml";

    return xmldata;
    }

};
