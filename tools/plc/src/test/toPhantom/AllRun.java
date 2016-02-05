package test.toPhantom;

public class AllRun {

	public static void main(String[] args) {

		runOne(new ArrayAccess(), "ArrayAccess" );
		runOne(new ArrayAssigns(), "ArrayAssigns" );

		System.out.println("Done tests");
	}

	public static void runOne(Testable t, String name)
	{
		int err = t.runTest();
		if( err != 0 )
			System.out.println("Err "+err+" in "+name);
	}

}
