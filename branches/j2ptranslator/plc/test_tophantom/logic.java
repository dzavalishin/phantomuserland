package translation.java.test;

class logic {

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