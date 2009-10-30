package .translation.phantom.test;

import .internal.int;
import .internal.string;
import .translation.phantom.test.a;
import .translation.java.test.b;

class obj_test {

    .internal.int returnInt() {
        var obj : .translation.phantom.test.a;
        obj = new .translation.phantom.test.a();
	  return obj.returnInt();
    }

    .internal.string returnStr() {
        var obj : .translation.phantom.test.a;
        obj = new .translation.phantom.test.a();
	  return obj.returnStr();
    }

    .internal.string returnObj() {
        var obj : .translation.java.test.b;
        obj = new .translation.java.test.b();
	  return obj;
    }



    .internal.int argInt() {
        var obj : .translation.phantom.test.a;
        obj = new .translation.phantom.test.a();
	  return obj.argInt(777);
    }

    .internal.string argStr() {
        var obj : .translation.phantom.test.a;
        obj = new .translation.phantom.test.a();
	  return obj.argStr("ok");
    }

    .internal.string argObj() {
	  return "error";
    }


    .internal.int firstArgInt() {
        var obj : .translation.phantom.test.a;
        obj = new .translation.phantom.test.a();
	  return obj.firstArgInt(10, 3);
    }

    .internal.int secondArgInt() {
        var obj : .translation.phantom.test.a;
        obj = new .translation.phantom.test.a();
	  return obj.secondArgInt(3, 10);
    }

    .internal.string firstArgStr() {
        var obj : .translation.phantom.test.a;
        obj = new .translation.phantom.test.a();
	  return obj.firstArgStr("ok", "error");
    }

    .internal.string secondArgStr() {
        var obj : .translation.phantom.test.a;
        obj = new .translation.phantom.test.a();
	  return obj.secondArgStr("error", "ok");
    }

    .internal.int addInt() {
        var obj : .translation.phantom.test.a;
        obj = new .translation.phantom.test.a();
	  return obj.addInt(7, 3);
    }

    .internal.int subIntPositive() {
        var obj : .translation.phantom.test.a;
        obj = new .translation.phantom.test.a();
	  return obj.subInt(12, 2);
    }

    .internal.int subIntNegative() {
        var obj : .translation.phantom.test.a;
        obj = new .translation.phantom.test.a();
	  return obj.subInt(2, 12);
    }

    .internal.int mulInt() {
        var obj : .translation.phantom.test.a;
        obj = new .translation.phantom.test.a();
	  return obj.mulInt(2, 5);
    }

    .internal.int divInt() {
        var obj : .translation.phantom.test.a;
        obj = new .translation.phantom.test.a();
	  return obj.divInt(30, 3);
    }

    .internal.int divIntLost() {
        var obj : .translation.phantom.test.a;
        obj = new .translation.phantom.test.a();
	  return obj.divInt(31, 3);
    }

    .internal.int expression() {
        var obj : .translation.phantom.test.a;
        obj = new .translation.phantom.test.a();
	  return obj.expression(9, 10, 21, 3, 6);
    }

};