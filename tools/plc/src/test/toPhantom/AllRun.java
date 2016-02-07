package test.toPhantom;

public class AllRun {
	int nErr = 0;
	
	public void runAll()
	{
		
		runOne(new ArrayAccess(), "ArrayAccess" );
		runOne(new ArrayAssigns(), "ArrayAssigns" );
		runOne(new ArraySimple1(), "ArraySimple1" );
		runOne(new Assigns(), "Assigns" );
		runOne(new Arrays(), "Arrays" );
		runOne(new Strings(), "Strings" );
		runOne(new Loops(), "Loops" );

		System.out.println("Done tests, "+nErr+" errors");
		
	}
	
	public void runOne(Testable t, String name)
	{
		int err = t.runTest();
		if( err != 0 )
		{
			System.out.println("Err "+err+" in "+name);
			nErr++;
		}
		else
			System.out.println("Finished test "+name);
	}

	public static void main(String[] args) 
	{
		AllRun ar = new AllRun();
		ar.runAll();
	}


}
