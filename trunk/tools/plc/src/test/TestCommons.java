package test;

import static org.junit.Assert.fail;
import ru.dz.plc.PlcMain;

public class TestCommons {

	public static void compileIjternalPhantomClasses()
	{
		
		compile("test/ph/internal/stub.object.ph");
		compile("test/ph/internal/stub.class.ph");
		compile("test/ph/internal/stub.thread.ph");
		
		compile("test/ph/internal/internal.string.ph");
		compile("test/ph/internal/internal.class.ph");
		compile("test/ph/internal/internal.object.ph");
		compile("test/ph/internal/internal.int.ph");
		
	}
	
	static void compile(String src)
	{
		String[] args = new String[3];
		
		args[0] = "-Itest/pc";
		args[1] = "-otest/pc";
		args[2] = src;
		
		try {
			
		
		if( PlcMain.go(args) )
			fail("Compile failed");
		}
		catch( Throwable o )
		{
			fail("Compile failed: "+o.toString());
		}
	}
	
	
}
