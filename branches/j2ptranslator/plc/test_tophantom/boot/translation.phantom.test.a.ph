package .translation.phantom.test;

import .internal.int;
import .internal.string;

class a {
    .internal.int returnInt() {
        return 777;
    }

    .internal.string returnStr() {
        return "ok";
    }

    .internal.int argInt(var arg : .internal.int) {
        return arg;
    }

    .internal.string argStr(var arg : .internal.string) {
        return arg;
    }

    .internal.int firstArgInt(var arg1 : .internal.int, var arg2 : .internal.int) {
        return arg1;
    }

    .internal.int secondArgInt(var arg1 : .internal.int, var arg2 : .internal.int) {
        return arg2;
    }

    .internal.string firstArgStr(var arg1 : .internal.string, var arg2 : .internal.string) {
        return arg1;
    }

    .internal.string secondArgStr(var arg1 : .internal.string, var arg2 : .internal.string) {
        return arg2;
    }

    .internal.int addInt(var arg1 : .internal.int, var arg2 : .internal.int) {
        return arg1 + arg2;
    }

    .internal.int subInt(var arg1 : .internal.int, var arg2 : .internal.int) {
        return arg1 - arg2;
    }

    .internal.int mulInt(var arg1 : .internal.int, var arg2 : .internal.int) {
        return arg1 * arg2;
    }

    .internal.int divInt(var arg1 : .internal.int, var arg2 : .internal.int) {
        return arg1 / arg2;
    }

    .internal.int expression(var arg1 : .internal.int, var arg2 : .internal.int,
                             var arg3 : .internal.int, var arg4 : .internal.int,
                             var arg5 : .internal.int) {
	  return 700 + arg1 * arg2 - arg3 / arg4 - arg5;
    }
};