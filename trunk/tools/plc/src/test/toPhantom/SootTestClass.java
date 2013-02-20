package test.toPhantom;

public class SootTestClass {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		String arg0 = args[0];
		
	}

	int strLen(String s)
	{
		int i = s.length()-1;
		return i+1;
	}
	
	
	String retString()
	{
		String sa[] = new String[2];
		
		int j;
		for( j = 0; j < sa.length; j++)
		{
			sa[j] = String.format("s%d", j);
		}
			
		int i = strLen(sa[0]);
	
		return sa[1];
	}

	String stringMethod()
	{
		int i = 10;
		float j = 0;
		while(i-- > 0)
			j += 3.3;
		return "aaa";
	}
	
	void voidMethod()
	{
		System.out.print("TestPrint");
		System.out.print(stringMethod());
	}
	
}
