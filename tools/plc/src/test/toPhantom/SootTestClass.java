package test.toPhantom;


public class SootTestClass {
	int classIntField = 0;
	
    /**
     * @param args
     */
    public static void main(String[] args) {
        String arg0 = args[0];

        int i = args.length;
        int len = 0;
        

        switch(i)
        {
        case 0:         len = 0; break;
        case 1:         len = strLen(args[0] ); break;
        case 2:         len = strLen(args[1] ); break;
        case 3:         len = strLen(args[2] ); break;
        }

        int v2 = 0;

        switch(len)
        {
        case 12: v2 = 24; break;
        case 70: v2 = 140; break;
        case 50: v2 = 100; break;
        default: v2 = 0; break;
        }


    }

    // seems to be compiled ok
    static int strLen(String s)
    {
        int i = s.length()-1;
        return i+1;
    }

    String retString()
    {
        String sa[] = new String[2];

        int j;
        for( j = 0; j < sa.length; j++)
        {
            sa[j] = String.format("s%d", j);
        }

        int i = strLen(sa[0]);
        classIntField = i;

        return sa[1];
    }

    // seems to be compiled ok
    String stringMethod()
    {
        /* floats not impl
         int i = 10;
         float j = 0;
         while(i-- > 0)
         j += 3.3;
         */
        return "aaa";
    }

    @Deprecated
    void voidMethod()
    {
        classIntField = 0;
        
        //Inner i = new Inner();
        //		System.out.print("TestPrint");
        //		System.out.print(stringMethod());
    }

    /* do not support inner classes yet
    public class Inner {
        public void run(){
        	int j = classIntField;
        	System.out.println(j);
        }
    }
    */
}
