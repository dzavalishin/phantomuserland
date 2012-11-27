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
		
		TestCommons.compileIjternalPhantomClasses();

	}

	/**
	 * @throws java.lang.Exception
	 */
	@After
	public void tearDown() throws Exception {
	}

	void compile(String src)
	{
		TestCommons.compile(src);
	}

	
	
	@Test
	public void testExtendsImplements() {		compile("test/plc/test_extends_implemens.ph");	}

	@Test
	public void testMisc() 				{		compile("test/plc/test_misc.ph");	}

	@Test
	public void testImport() 			{		compile("test/plc/test_import.ph");	}

	
	
	
	
}
