//package translation.java.test;

class a {
    int returnInt() {
        return 777;
    }

    String returnStr() {
        return "ok";
    }

    b returnObj() {
        b obj = new b();
	  return obj;
    }



    int argInt(int arg ) {
        return arg;
    }

    String argStr(String arg) {
        return arg;
    }

    String argObj(b arg) {
        return arg.returnStr();
    }



    int firstArgInt(int arg1, int arg2) {
        return arg1;
    }

    int secondArgInt(int arg1, int arg2) {
        return arg2;
    }

    String firstArgStr(String arg1, String arg2) {
        return arg1;
    }

    String secondArgStr(String arg1, String arg2) {
        return arg2;
    }



    int addInt(int arg1, int arg2) {
        return arg1 + arg2;
    }

    int subInt(int arg1, int arg2) {
        return arg1 - arg2;
    }

    int mulInt(int arg1, int arg2) {
        return arg1 * arg2;
    }

    int divInt(int arg1, int arg2) {
        return arg1 / arg2;
    }

    int expression(int arg1, int arg2,
                   int arg3, int arg4,
                   int arg5) {
	  return 700 + arg1 * arg2 - arg3 / arg4 - arg5;
    }

}