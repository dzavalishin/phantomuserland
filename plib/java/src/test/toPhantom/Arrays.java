package test.toPhantom;

public class Arrays implements Testable {

	//@Override
	public int runTest() 
	{
		{
			int []m = {1, 2, 3};

			if( m[0] != 1 ) return 1;
			if( m[1] != 2 ) return 2;
			if( m[2] != 3 ) return 3;
		}

		// TODO ERROR multidim array
		/*
		{
			// TODO ERROR test double
			//double [][]m = {{1, 2, 3}, {2, 5, 6}, {3, 6, 9}};
			int [][]m = {{1, 2, 3}, {2, 5, 6}, {3, 6, 9}};

			if( m[0][0] != 1 ) return 4;
		}
		*/

		return 0;
	}

}
