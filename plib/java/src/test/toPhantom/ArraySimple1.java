package test.toPhantom;

public class ArraySimple1 implements Testable {

	//@Override
	public int runTest() {

		int[] ia = new int[101];
        
		for (int i = 0; i < ia.length; i++)
            ia[i] = i;
		
        int sum = 0;
        
        for (int i = 0; i < ia.length; i++)
            sum += ia[i];
        
		return (sum == 5050) ? 0 : sum;
	}

}
