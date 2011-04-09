package translation.java.test;

class switch_test {

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


    String lookupswitch_break() {
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

    String lookupswitch_const_break() {
        String result = "failure";

        switch(22)	{
            case 11: { result = "failure11"; break; }
            case 22: { result = "ok"; break; }
            case 33: { result = "failure33"; break; }
            default: { result = "failureDefault"; break; }
        }

        return result;
    }

    String tableswitch() {
        int i = 13;
        String result = "failure";

        switch(i)	{
            case 11:  result = "failure11";
            case 12:  result = "failure12";
            case 13:  result = "ok";
        }

        return result;
    }

    String tableswitch_last() {
        int i = 11;
        String result = "failure";

        switch(i)	{
            case 11:  result = "failure11";
            case 12:  result = "failure12";
            case 13:  result = "ok";
        }

        return result;
    }

    String tableswitch_default() {
        int i = 44;
        String result = "failure";

        switch(i)	{
            case 11:  result = "failure11";
            case 12:  result = "failure12";
            case 13:  result = "failure13";
            default:  result = "ok";
        }

        return result;
    }

    String tableswitch_last_default() {
        int i = 11;
        String result = "failure";

        switch(i)	{
            case 11:  result = "failure11";
            case 12:  result = "failure12";
            case 13:  result = "failure13";
            default:  result = "ok";
        }

        return result;
    }

    String tableswitch_break() {
        int i = 12;
        String result = "failure";

        switch(i)	{
            case 11:  result = "failure11"; break;
            case 12:  result = "ok"; break;
            case 13:  result = "failure13"; break;
            default:  result = "failureDefault"; break;
        }

        return result;
    }

    String tableswitch_const_break() {
        String result = "failure";

        switch(12)	{
            case 11:  result = "failure11"; break;
            case 12:  result = "ok"; break;
            case 13:  result = "failure13"; break;
            default:  result = "failureDefault"; break;
        }

        return result;
    }

};