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

	void compile(String src)
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

	
	
	@Test
	public void testExtendsImplements() {		compile("test/plc/test_extends_implemens.ph");	}

	@Test
	public void testMisc() 				{		compile("test/plc/test_misc.ph");	}

	@Test
	public void testImport() 			{		compile("test/plc/test_import.ph");	}

	
	
	
	
}
