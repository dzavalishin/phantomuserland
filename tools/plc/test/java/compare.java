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



    String if_acmpeq(String arg) {
        if (arg != arg) return "failure";
        else return "ok";
    }
    String if_acmpne(String arg) {
        if (arg == arg) return "ok";
        else return "failure";
    }


}