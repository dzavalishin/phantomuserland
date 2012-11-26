package translation.java.test;

class loop {

    public int loop_for() {
        int result=0;
        for (int i=0; i<7; i=i+1) {
            result = result + 1;
        }
        return result;
    }
}