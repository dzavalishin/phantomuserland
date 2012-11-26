package translation.java.test;


class obj_test {

    int returnInt() {
        a obj = new a();
	  return obj.returnInt();
    }

    String returnStr() {
        a obj = new a();
	  return obj.returnStr();
    }

    b returnObj() {
        a obj = new a();
	  return obj.returnObj();
    }




    int argInt() {
        a obj = new a();
	  return obj.argInt(777);
    }

    String argStr() {
        a obj = new a();
        return obj.argStr("ok");
    }

    String argObj() {
        a obj = new a();
        b argObj = new b();
        return obj.argObj(argObj);
    }



    int firstArgInt() {
        a obj = new a();
        return obj.firstArgInt(10, 3);
    }

    int secondArgInt() {
        a obj = new a();
        return obj.secondArgInt(3, 10);
    }

    String firstArgStr() {
        a obj = new a();
        return obj.firstArgStr("ok", "error");
    }

    String secondArgStr() {
        a obj = new a();
        return obj.secondArgStr("error", "ok");
    }



    int addInt() {
        a obj = new a();
	  return obj.addInt(7, 3);
    }

    int subIntPositive() {
        a obj = new a();
	  return obj.subInt(12, 2);
    }

    int subIntNegative() {
        a obj = new a();
	  return obj.subInt(2, 12);
    }

    int mulInt() {
        a obj = new a();
	  return obj.mulInt(2, 5);
    }

    int divInt() {
        a obj = new a();
	  return obj.divInt(30, 3);
    }

    int divIntLost() {
        a obj = new a();
        return obj.divInt(31, 3);
    }

    int expression() {
        a obj = new a();
        return obj.expression(9, 10, 21, 3, 6);
    }





    int getFieldInt() {
        field obj = new field();
	  return obj.getFieldInt();
    }

    void setFieldInt() {
        field obj = new field();
	  obj.setFieldInt(777);
    }

    int getSetFieldInt() {
        field obj = new field();
	  obj.setFieldInt(999);
	  return obj.getFieldInt();
    }

/*
    void accessSetInt() {
        field obj = new field();
	  obj.fieldInt = 777;
    }

    int accessGetSetInt() {
        field obj = new field();
	  obj.fieldInt = 999;
	  return obj.fieldInt;
    }
*/
}