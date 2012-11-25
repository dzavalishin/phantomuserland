/**
 * 
 */
package test;

import static org.junit.Assert.*;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import ru.dz.plc.PlcMain;

/**
 * @author dz
 *
 */
public class RunCompiler {

	/**
	 * @throws java.lang.Exception
	 */
	@Before
	public void setUp() throws Exception {
		
		compile("test/ph/internal/stub.object.ph");
		compile("test/ph/internal/stub.class.ph");
		compile("test/ph/internal/stub.thread.ph");
		
		compile("test/ph/internal/internal.string.ph");
		compile("test/ph/internal/internal.class.ph");
		compile("test/ph/internal/internal.object.ph");
		compile("test/ph/internal/internal.int.ph");

	}

	/**
	 * @throws java.lang.Exception
	 */
	@After
	public void tearDown() throws Exception {
	}

	@Test
	public void test() {
		
		compile("test/plc/test_extends_implemens.ph");
		
		//fail("Not yet implemented");
		
	}

	
	void compile(String src)
	{
		String[] args = new String[3];
		
		args[0] = "-Itest/pc";
		args[1] = "-otest/pc";
		args[2] = src;
		
		PlcMain.main(args);
		/*
		try { PlcMain.main(args); }
		catch( Throwable o )
		{
			fail("Compile failed: "+o.toString());
		}*/
	}
	
}
