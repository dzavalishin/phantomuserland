package test;

/*
import static org.junit.Assert.*;

import java.io.IOException;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import ru.dz.plc.util.PlcException;

public class RunJava2PhantomTest {

	//private static final String jc = "G:/Program Files/Java/jdk1.7.0_03/bin/javac.exe";
	//private static final String jc = "cmd /c javac -verbose -cp test/class ";
	private static final String jc = "cmd /c javac -cp test/class -d test/class ";
	
	@Before
	public void setUp() throws Exception {
		
		System.setProperty("toba.class.path","test/class");

		TestCommons.compileIjternalPhantomClasses();
		
	}

	@After
	public void tearDown() throws Exception {
	}

	@Test
	public void test() throws IOException, PlcException, InterruptedException {
		translate("b");
		translate("a");	
	}

	
	private void translate(String basename) throws IOException, InterruptedException, PlcException {
		
		//basename = "test/java/"+basename;
		
		translateJava2Class("test/java/"+basename);
		//translateJava2Phantom("test/class/translation/java/test/"+basename);
		translateJava2Phantom("test/class/"+basename);

		
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
		String[] args = new String[4];

		args[0] = "-c"; // phantom .pc class path
		args[1] = "test/pc"; 

		args[2] = "-d9";
		args[3] = basename+".class";

		ru.dz.jpc.tophantom.Trans.main(args);
	}
	
}
*/
