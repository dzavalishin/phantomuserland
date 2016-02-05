package test.toPhantom;

public class AllRun {
	static int nErr = 0;
	public static void main(String[] args) {

		runOne(new ArrayAccess(), "ArrayAccess" );
		runOne(new ArrayAssigns(), "ArrayAssigns" );
		runOne(new ArraySimple1(), "ArraySimple1" );

		System.out.println("Done tests, "+nErr+" errors");
	}

	public static void runOne(Testable t, String name)
	{
		int err = t.runTest();
		if( err != 0 )
		{
			System.out.println("Err "+err+" in "+name);
			nErr++;
		}
	}

}
