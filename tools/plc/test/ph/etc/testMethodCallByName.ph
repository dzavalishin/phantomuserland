/**
 *
 * Phantom OS - Phantom language library
 *
 * @author AnZuev(anzuev@bk.ru)
 * This file is used to test method calling by name in a way
   obj.*(methodName)(arg1, arg2);
 **/

package .ru.dz.phantom.system;


class Test{
    var val: int;

    void exec(var val1:int, var val2:int){
        return val1 + val2;
    }
};

class MethodByNameCalling
{
    var val1: Test;


    void test(){
        val1 = new Test();
        val1.exec(1, 2);
        val1.*("exec")(1, 2);

    }
};

