/**
 * 
 */
package ru.dz.soot;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.logging.Logger;

import ru.dz.plc.PlcMain;
import ru.dz.plc.compiler.ClassMap;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.util.PlcException;
import soot.Scene;
import soot.SootClass;
import soot.SootMethod;

/**
 * @author dz
 *
 */
public class SootMain {
	private static int errors = 0;
	
	static Logger log = Logger.getLogger(SootMain.class.getName());
    static ClassMap classes = ClassMap.get_map();


	/**
	 * @param args
	 * @throws PlcException 
	 * @throws IOException 
	 */
	public static void main(String[] args) throws PlcException, IOException {
		String phantomClassPath = "test/pc";
		
		PlcMain.addClassFileSearchParh(new File(phantomClassPath));
		PlcMain.setOutputPath(phantomClassPath);
		
		classes.do_import(".internal.object");			
		classes.do_import(".internal.int");

		try { classes.do_import(".internal.string"); } finally {}
		
		//String cp = "bin;../bin;lib/rt_6.jar";
		String cp = 
				"bin"+
				File.pathSeparator+
				"../bin"+
				File.pathSeparator+
				"lib/rt_6.jar";
		//System.setProperty("soot.class.path", cp);
		//say(cp);
		
		Scene.v().setSootClassPath(cp);
	
		doClass("test.toPhantom.SootTestClass");
	
		if(errors > 0)
		{
			say("Compile errors, stopped");
			return;
		}
	}

	
	private static void doClass(String cn) throws PlcException, IOException
	{
		SootClass c = Scene.v().loadClassAndSupport(cn);
		if( c.isPhantom() )
		{
			die("Not loaded");
		}

		PhantomClass pc = new PhantomClass(convertClassName(c.getName()));
		
		List<SootMethod> mlist = c.getMethods();
		
		for( SootMethod m : mlist )
		{
			SootMethodTranslator mt = new SootMethodTranslator(m,pc);
			//doMethod(m);
			mt.process();
		}
		
		classes.add(pc);

		say("Generate Phantom code for "+pc.getName());
		classes.codegen();
	}
	
	private static String convertClassName(String name) {
		name = "."+name;
		say("Class "+name);
		return name;
	}

	private static void die(String string) {
		System.err.println(string);
		System.exit(33);
	}

	static void say(String string) {
		System.err.println(string);
	}

	public static void error(String string) {
		say("Error: "+string);
		errors ++;
	}

}
