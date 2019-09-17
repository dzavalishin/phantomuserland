package .ru.dz.test;

// it is impossible to use untyped var in numeric expr, but possible as object.

class base
{
    var unt1;

    void test1( var unt2)
    {
        //var unt3; // syntax error - TODO infer ver type?
        
        unt1 = "a";
        unt2 = "a";
        //unt3 = 1;
    }

    
};
