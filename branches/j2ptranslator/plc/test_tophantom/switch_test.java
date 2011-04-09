package translation.java.test;

class switch_test {
/*
    String tableswitch() {
        int i = 1;
        String result = "failure";

        switch(i)	{
            case 1:  result = "ok";
            case 2:  result = "ok2";
            case 3:  result = "ok3";
            default:  result = "failure default";
        }

        return result;
    }
*/
    String lookupswitch() {
        int i = 33;
        String result = "failure";

        switch(i)	{
            case 11:  result = "failure11";
            case 22:  result = "failure22";
            case 33:  result = "ok";
        }

        return result;
    }

    String lookupswitch_last() {
        int i = 11;
        String result = "failure";

        switch(i)	{
            case 11:  result = "failure11";
            case 22:  result = "failure22";
            case 33:  result = "ok";
        }

        return result;
    }

    String lookupswitch_default() {
        int i = 44;
        String result = "failure";

        switch(i)	{
            case 11:  result = "failure11";
            case 22:  result = "failure22";
            case 33:  result = "failure33";
            default:  result = "ok";
        }

        return result;
    }

    String lookupswitch_last_default() {
        int i = 11;
        String result = "failure";

        switch(i)	{
            case 11:  result = "failure11";
            case 22:  result = "failure22";
            case 33:  result = "failure33";
            default:  result = "ok";
        }

        return result;
    }


    String lookupswitch_brake() {
        int i = 22;
        String result = "failure";

        switch(i)	{
            case 11: { result = "failure11"; break; }
            case 22: { result = "ok"; break; }
            case 33: { result = "failure33"; break; }
            default: { result = "failureDefault"; break; }
        }

        return result;
    }

};