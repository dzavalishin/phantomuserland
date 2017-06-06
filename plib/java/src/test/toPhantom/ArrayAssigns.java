package test.toPhantom;

public class ArrayAssigns implements Testable {

	public int runTest()
	{
        int [] arr = new int [] {1, 2, 3};
        int i = 0;
        
        D d = new D(9);
        arr[i++] += d.height;
        arr[i++] += d.height;
        
        if( arr[0] != 10 ) return 1;
        if( arr[1] != 11 ) return 2;
        if( arr[2] != 3 ) return 3;
        
        //S s = new S(8);
        //D d = new D(9);

        //s.height += d.height;
		
		return 0;
	}
}
