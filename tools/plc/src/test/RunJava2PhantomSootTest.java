package test;

import static org.junit.Assert.*;

import java.io.IOException;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import ru.dz.plc.util.PlcException;


public class RunJava2PhantomSootTest {

	private static final String jc = "cmd /c javac -cp test/class -d test/class ";
	
	@Before
	public void setUp() throws Exception 
	{
		
		TestCommons.compileIjternalPhantomClasses();
		
	}

	@After
	public void tearDown() throws Exception {
	}

	@Test
	public void test() throws IOException, PlcException, InterruptedException {
		translate("test.toPhantom.SootTestClass");
		//translate("a");
		
	}

	
	private void translate(String basename) throws IOException, InterruptedException, PlcException {
		
		//basename = "test/java/"+basename;
		
		//translateJava2Class("src/test/toPhantom/"+basename);
		translateJava2Phantom(basename);

		
	}

	private void translateJava2Class(String basename) throws IOException, InterruptedException {
		//Runtime.getRuntime().exec("cmd.exe /c pwd >aaa");
		
		String cmd = jc+" "+basename+".java 2>javac.log";
		System.out.println(cmd);
		Process exec = Runtime.getRuntime().exec(cmd);
		int rc = exec.waitFor();
		if( rc != 0 )
			fail("javac "+basename+" failed: "+rc);
	}

	private void translateJava2Phantom(String basename) throws IOException, PlcException
	{
		String[] args = new String[1];
		args[0] = basename;

		ru.dz.soot.SootMain.main(args);
	}
	
}
