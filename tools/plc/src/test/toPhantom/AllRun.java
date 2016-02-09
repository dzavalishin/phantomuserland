package test.toPhantom;

public class AllRun {
	int nErr = 0;
	private IPhantomPrinter p;
	
	public void runAll()
	{
		runOne(new Assigns(), "Assigns" );
		runOne(new Loops(), "Loops" );
		
		runOne(new ArrayAccess(), "ArrayAccess" );
		runOne(new ArrayAssigns(), "ArrayAssigns" );
		runOne(new ArraySimple1(), "ArraySimple1" );
		runOne(new Arrays(), "Arrays" );
		runOne(new Strings(), "Strings" );

		//p.print("Done tests, "+nErr+" errors");
		p.print("Done tests");
	}
	
	public void runOne(Testable t, String name)
	{
		int err = t.runTest();
		if( err != 0 )
		{
			//System.out.println("Err "+err+" in "+name);
			p.print("Error");
			//p.print(err.toString());
			p.print(name);
			nErr++;
		}
		else
		{
			p.print("Finished test ");
			p.print(name);
		}
	}

	/*
	public static void main(String[] args) 
	{
		AllRun ar = new AllRun();
		ar.runAll();
	}
	*/
	
	public void setPrinter(IPhantomPrinter p) {
		this.p = p;
	}


}
