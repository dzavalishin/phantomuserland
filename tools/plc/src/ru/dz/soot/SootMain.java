/**
 * 
 */
package ru.dz.soot;

import java.io.File;
import java.io.IOException;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.logging.Logger;

import ru.dz.plc.PlcMain;
import ru.dz.plc.compiler.ClassMap;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;
import soot.Scene;
import soot.SootClass;
import soot.SootField;
import soot.SootMethod;
import soot.options.Options;
import soot.tagkit.InnerClassTag;
import soot.tagkit.Tag;
import soot.util.Chain;

/**
 * @author dz
 *
 */
public class SootMain {
	private static int errors = 0;
	private static int warnings = 0;
	
	static Logger log = Logger.getLogger(SootMain.class.getName());
    static ClassMap classes = ClassMap.get_map();
	private static Set<String> classesToDo = new HashSet<String>();



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
				"."+
				File.pathSeparator+				"bin"+
				File.pathSeparator+				"../bin"+
				File.pathSeparator+				"lib/rt_6.jar"+
				File.pathSeparator+				"../lib/rt_6.jar";
		//System.setProperty("soot.class.path", cp);
		//say(cp);
		
		Scene.v().setSootClassPath(cp);
		Options.v().set_keep_line_number(true);
	
		if(args.length == 0)
			doClass("test.toPhantom.SootTestClass");
		else
		{
			for( String a : args )
				doClass(a);
		}

		for( String iClass : classesToDo )
		{
			String cName = iClass.replace('/', '.');
			say("Now process inner class "+cName);
			doClass(cName);
		}
		
		if(errors > 0)
		{
			say("Compile errors, stopped");
			return;
		}
		
		if( warnings > 0 )
			say(String.format("%d warnings\n", warnings ));

		say("Generate Phantom code");
		classes.codegen();
	}

	
	private static void doClass(String cn) throws PlcException, IOException
	{
		SootClass c = Scene.v().loadClassAndSupport(cn);
		if( c.isPhantom() )
		{
			die("Not loaded "+c.getName());
		}

		for( Tag t : c.getTags() )
		{
			say("class tag '"+t+"' tag "+t.getClass()+" name "+t.getName());
			
			if (t instanceof InnerClassTag) {
				InnerClassTag iClass = (InnerClassTag) t;
				classesToDo .add( iClass.getInnerClass() );
			}
			
		}
		
		PhantomClass pc = new PhantomClass(convertClassName(c.getName()));
		
		Chain<SootField> fields = c.getFields();
		for( SootField f : fields )
		{
			PhantomType type = SootExpressionTranslator.convertType(f.getType());
			say("add field '"+f.getName()+"' type '"+type+"' to "+pc.getName());
			pc.addField(f.getName(), type);
		}
		
		List<SootMethod> mlist = c.getMethods();
		
		for( SootMethod m : mlist )
		{
			SootMethodTranslator mt = new SootMethodTranslator(m,pc);
			//doMethod(m);
			mt.process();
		}
		
		classes.add(pc);

		//say("Generate Phantom code for "+pc.getName());
		//classes.codegen();
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
		System.out.println(string);
	}

	public static void error(String string) {
		System.err.println("Error: "+string);
		errors ++;
	}


	public static void error(Throwable e) {
		error("Exception "+e);		
	}


	public static void warning(String string) {
		System.err.println("Warning: "+string);
		warnings ++;
	}

}
