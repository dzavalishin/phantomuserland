/**
 * 
 */
package test;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import ru.dz.plc.compiler.ClassMap;


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
		ClassMap.get_map().clear();
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

	@Test
	public void testShell() 			{		compile("test/ph/ru.dz/ru.dz.phantom.system.shell.ph" ); }
	
	//@Test
	//public void testMethodCall()		{ 		compile("test/ph/etc/testMethodCallByName.ph");
	
	@Test
	public void testUntypedVar()		{ 		compile("test/plc/test_untyped_var.ph");
	}
	
	@Test
	public void testBaseFieldAccess()	{ 		compile("test/plc/test_base_field_access.ph");
	}
	
	@Test
	public void testCtorParam()		{ 		compile("test/plc/test_ctor_param.ph");
	}
	
}
