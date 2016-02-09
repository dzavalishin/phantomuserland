package test.toPhantom;

public class ArrayAccess implements Testable {

	public int runTest()
	{
        int [] arr = new int [] {3, 4, 2, 1};
        if( arr[3] != 1 ) return 1; // err

		int [] a = new int[7];
		a[0] = 0;
		
		for (int i = 1; i < 7; i++){
			a[i] = i;
		}

		if( a[4] != 4 ) return 2;

		for (int j = 0; j < 7; j++){
			if( a[j] != j )
				return 10+j;
		}
        
        return 0;
	}
	
}
