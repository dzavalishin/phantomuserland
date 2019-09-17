package test.toPhantom;

public class Assigns 
	implements Testable 
{
    public int j1 = 4;
    public int y1 = j1 * 4;
    

//	@Override
	public int runTest() 
	{

		{
	        int x = 9;
	        int y = 10;
	        int z = 0;

	        z += y;
	        z -= x;
			
	        if( z != 1 )	return 1; // ok		
	        if( x != 9 )	return 2;		
	        if( y != 10 )	return 3;		
		}
		
		{
			int i1 = 8;
			int x1 = i1 * i1;

			if( x1 != 64 ) return 4; 
			if( y1 != 16 ) return 5; 			
		}

		{
		    
	        int x = 9;
	        int y = 10;

	        boolean b = !(x > y);
	        int b2 = ~x;
			
	        if( b != true)	return 6;
	        if( b2 != -10)	return 7;
		}
		
		{	// assign condition
	        int l1 = 4;
	        int res = 5;

	        boolean ok = res == l1;
	        if( ok != false )	return 8;
		}
		
		{
	        int side;
	        int otherside;
	        int plycnt = 7;
	        
	        side = (otherside = plycnt & 1) ^ 1;   
			
	        if( otherside != 1 )	return 9;
	        if( side != 0 )			return 10;
		}

		// TODO ERROR long type test
		/*
		{	// assign condition
	        long l1 = 4L;
	        long res = 5L;

	        boolean ok = res == l1;
	        if( ok != false )	return 11;
		}
		*/
		
		return 0;
	}

}
