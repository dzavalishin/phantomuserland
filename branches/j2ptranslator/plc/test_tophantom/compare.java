package translation.java.test;

class compare {

    String if_icmple(int arg) {
        if (arg > 5) return "ok";
        return "error";
    }

    String if_icmplt(int arg) {
        if (arg >= 5) return "ok";
        return "error";
    }

    String if_icmpge(int arg) {
        if (arg < 11) return "ok";
        return "error";
    }

    String if_icmpgt(int arg) {
        if (arg <= 10) return "ok";
        return "error";
    }

    String if_icmpne(int arg) {
        if (arg == 10) return "ok";
        return "error";
    }

    String if_icmpeq(int arg) {
        if (arg != 11) return "ok";
        return "error";
    }



    String ifle(int arg) {
        if (arg > 0) return "ok";
        return "error";
    }

    String iflt(int arg) {
        if (arg >= 0) return "ok";
        return "error";
    }

    String ifge(int arg) {
        if (arg < 0) return "ok";
        return "error";
    }

    String ifgt(int arg) {
        if (arg <= 0) return "ok";
        return "error";
    }

    String ifeq(int arg) {
        if (arg != 0) return "ok";
        return "error";
    }

    String ifne(int arg) {
        if (arg == 0) return "ok";
        return "error";
    }


    String if_icmple_else(int arg) {
        if (arg > 5) return "if";
        else return "else";
    }

    String if_icmplt_else(int arg) {
        if (arg >= 5) return "if";
        else return "else";
    }

    String if_icmpge_else(int arg) {
        if (arg < 5) return "if";
        else return "else";
    }

    String if_icmpgt_else(int arg) {
        if (arg <= 5) return "if";
        else return "else";
    }



    String if_icmple_elseif_else(int arg) {
        if (arg > 5) return "if";
        else if (arg > 3) return "else if";
        else return "else";
    }


    int if_icmple_all(int arg) {
        arg = arg + 1;
        if (arg > 20) {
            arg = arg + 200;
        }
        else if (arg > 10) {
            arg = arg + 100;
        }
        else { 
            arg = arg + 10;
        }
        arg = arg + 2;        
        return arg;
    }


    String ifnull(String arg) {
        if (arg != null) return "not null";
        else return "null";
    }

    String ifnonnull(String arg) {
        if (arg == null) return "null";
        else return "not null";
    }





    String logic_and_and(int arg) {
        if (arg > 1 && arg > 2) return "ok";
        else return "failure";
    }

    String logic_and_and2(int arg) {
        if (arg > 1 && arg > 2 && arg > 3) return "ok";
        else return "failure";
    }

    String logic_and_and_n(int arg) {
        if (arg > 1 && arg > 2) return "failure";
        else return "ok";
    }

    String logic_and_and2_n(int arg) {
        if (arg > 1 && arg > 2 && arg > 3) return "failure";
        else return "ok";
    }

    String logic_and_and_zero_n(int arg) {
        if (arg > 1 && arg/0 > 2) return "failure";
        else return "ok";
    }



    String logic_or_or(int arg) {
        if (arg > 2 || arg > 1) return "ok";
        else return "failure";
    }
    String logic_or_or2(int arg) {
        if (arg > 3 || arg > 2 || arg>1) return "ok";
        else return "failure";
    }
    String logic_or_or_n(int arg) {
        if (arg > 2 || arg > 1) return "failure";
        else return "ok";
    }
    String logic_or_or2_n(int arg) {
        if (arg > 3 || arg > 2 || arg>1) return "failure";
        else return "ok";
    }
    String logic_or_or_zero_n(int arg) {
        if (arg > 2 || arg/0 > 1) return "ok";
        else return "failure";
    }




    String logic_and(int arg) {
        if (arg > 1 & arg > 2) return "ok";
        else return "failure";
    }
    String logic_and_n(int arg) {
        if (arg > 1 & arg > 2) return "failure";
        else return "ok";
    }

 
    String logic_or(int arg) {
        if (arg > 1 | arg > 2) return "ok";
        else return "failure";
    }
    String logic_or_n(int arg) {
        if (arg > 1 | arg > 2) return "failure";
        else return "ok";
    }




    String logic_not(int arg) {
        if (!(arg > 1)) return "ok";
        else return "failure";
    }
    String logic_not_n(int arg) {
        if (!(arg > 1)) return "failure";
        else return "ok";
    }
}