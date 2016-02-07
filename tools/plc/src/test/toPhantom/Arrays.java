package test.toPhantom;

public class Arrays implements Testable {

	@Override
	public int runTest() 
	{
		{
			double [][]m = {{1, 2, 3}, {2, 5, 6}, {3, 6, 9}};

			if( m[0][0] != 1 ) return 1;
		}


		return 0;
	}

}
