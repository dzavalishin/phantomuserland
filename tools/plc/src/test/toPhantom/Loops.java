package test.toPhantom;

public class Loops implements Testable {

	@Override
	public int runTest() 
	{
		{

			/*for (int i = 0; i < 10; i++) 
			  {
		            System.out.println(i);
		        }*/

			int [] a = new int[100];
			
			for (int i = 0; i < 10; i++) 
			{
				for (int j = 0; j < 10; j++) 
				{
					//System.out.println(i*j);
					a[(i*10)+j] = i*j;
				}
			}

			for (int i = 0; i < 10; i++) 
			{
				for (int j = 0; j < 10; j++) 
				{
					//System.out.println(i*j);
					if( a[(i*10)+j] != i*j)
						return (i*10)+j;
				}
			}
		}

		{
	        int j = 9;
	        
	        for (int i = 1; i < 10; i++)
	        {
	            j = j*i;
	        }
	        
	        if( j != 3265920)
	        	return 101;
		}
		
		{
	        int i = 1;
	        for (;;) {
	        
	            if (i == 9) break;
	            if (i == 8) break;
	            i += i;
	        }

	        if( i != 8 )
	        	return 102;
		}

		{
	        int i = 4;
	        outer:while (i < 10 ) 
	        {
	            //System.out.println(i);
	            if (i == 8) break outer;
	            i = i + 2;
	        }
			
	        if( i != 8 )
	        	return 103;
	        
		}

		{
	        int i = 4;
	        while (i < 10 ) {
	            //System.out.println(i);
	            if (i == 6) break;
	            i = i + 2;
	        }
	        if( i != 6 )
	        	return 103;
	        
	    }

		return 0;
	}

}
