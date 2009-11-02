/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal: No
 * Preliminary: Yes
 *
 *
**/

package .ru.dz.phantom.system;

/**
 *
 * Some compiler/VM regression tests.
 *
**/

import .translation.phantom.test.obj_test;
import .translation.java.test.obj_test;
import .translation.java.test.b;
import .translation.java.test.compare;

attribute const * ->!;


class translation_regression_tests
{
    var boot_object;
    var boot_success : int;

    var p_obj : .translation.phantom.test.obj_test;
    var p_result_int : .internal.int;
    var p_result_str : .internal.string;
    var p_result_obj : .translation.java.test.b;
    var c_obj : .translation.java.test.compare;

    void run (var _boot_object @const ) {
        boot_object = _boot_object;
        boot_success = 0;

        print("Phantom language translation tests are running\n");

//        print("Run Phantom bytecode test\n");        
//        run_test(new .translation.phantom.test.obj_test());

        print("\n\nRun Java translated bytecode test\n");
        run_test(new .translation.java.test.obj_test());


        c_obj = new .translation.java.test.compare();

        log_str(c_obj.if_icmple(6), "if_icmple: ");
        log_str(c_obj.if_icmplt(6), "if_icmplt: ");
        log_str(c_obj.if_icmplt(5), "if_icmplt: ");

        log_str(c_obj.if_icmpge(10), "if_icmpge: ");
        log_str(c_obj.if_icmpgt(10), "if_icmpgt: ");
        log_str(c_obj.if_icmpgt(10), "if_icmpgt: ");

        log_str(c_obj.if_icmpne(10), "if_icmpne: ");
        log_str(c_obj.if_icmpeq(10), "if_icmpeq: ");


        log_str(c_obj.ifle(10), "ifle: ");

        log_str(c_obj.iflt(10), "iflt: ");
        log_str(c_obj.iflt(0), "iflt: ");

        log_str(c_obj.ifge(2-3), "ifle: ");

        log_str(c_obj.ifgt(2-3), "iflt: ");
        log_str(c_obj.ifgt(0), "iflt: ");

        log_str(c_obj.ifeq(1), "ifeq: ");
        log_str(c_obj.ifne(0), "ifne: ");

        log_str(c_obj.if_icmple_else(6), "if_icmple_else: ");
        log_str(c_obj.if_icmple_else(4), "if_icmple_else: ");

        log_str(c_obj.if_icmplt_else(5), "if_icmplt_else: ");
        log_str(c_obj.if_icmplt_else(4), "if_icmplt_else: ");

        log_str(c_obj.if_icmpge_else(4), "if_icmpge_else: ");
        log_str(c_obj.if_icmpge_else(6), "if_icmpge_else: ");

        log_str(c_obj.if_icmpgt_else(5), "if_icmpgt_else: ");
        log_str(c_obj.if_icmpgt_else(6), "if_icmpgt_else: ");


        log_str(c_obj.if_icmple_elseif_else(6), "if_icmple_elseif_else: ");
        log_str(c_obj.if_icmple_elseif_else(4), "if_icmple_elseif_else: ");
        log_str(c_obj.if_icmple_elseif_else(2), "if_icmple_elseif_else: ");


        log_str(c_obj.if_icmple_all(25), "if_icmple_all: ");
        log_str(c_obj.if_icmple_all(15), "if_icmple_all: ");
        log_str(c_obj.if_icmple_all(5), "if_icmple_all: ");



        print("\n\n");
    }

	// ---------------------------------------------------------------------
	// Helpers
	// ---------------------------------------------------------------------

	void print( var input : string )
	{
	    boot_object.16(input);
	}

    void log_int(var value : .internal.int, var result : .internal.int, var test_name : .internal.string) {
        print("\n    ");
        print(test_name);
        if (value != result) print("error");
        else print("ok");
    }
    void log_str(var result : .internal.string, var test_name : .internal.string) {
        print("\n    ");
        print(test_name);
        print(result);
    }


	// ---------------------------------------------------------------------
	// test bytecode
	// ---------------------------------------------------------------------

    void run_test(var obj : .internal.object) {

        p_obj = obj;
//        p_obj = new .translation.phantom.test.obj_test();

        p_result_int = p_obj.returnInt();
        log_int(777, p_result_int, "return int: ");

        p_result_str = p_obj.returnStr();
        log_str(p_result_str, "return string: ");

        p_result_obj = p_obj.returnObj();
        p_result_str = p_result_obj.returnStr();
        log_str(p_result_str, "return object: ");



        p_result_int = p_obj.argInt();
        log_int(777, p_result_int, "arg int: ");

        p_result_str = p_obj.argStr();
        log_str(p_result_str, "arg string: ");

        p_result_str = p_obj.argObj();
        log_str(p_result_str, "arg object: ");



        p_result_int = p_obj.firstArgInt();
        log_int(10, p_result_int, "first arg int: ");

        p_result_int = p_obj.secondArgInt();
        log_int(10, p_result_int, "second arg int: ");

        p_result_str = p_obj.firstArgStr();
        log_str(p_result_str, "first arg string: ");

        p_result_str = p_obj.secondArgStr();
        log_str(p_result_str, "second arg string: ");



        p_result_int = p_obj.addInt();
        log_int(10, p_result_int, "add int: ");

        p_result_int = p_obj.subIntPositive();
        log_int(10, p_result_int, "sub int positive: ");

        p_result_int = p_obj.subIntNegative();
        log_int(10-20, p_result_int, "sub int negative: ");

        p_result_int = p_obj.mulInt();
        log_int(10, p_result_int, "mul int: ");

        p_result_int = p_obj.divInt();
        log_int(10, p_result_int, "div int: ");

        p_result_int = p_obj.divIntLost();
        log_int(10, p_result_int, "div int lost: ");
 
        p_result_int = p_obj.expression();        
        log_int(777, p_result_int, "expression: ");


        print("\nDone\n");
    }
};
