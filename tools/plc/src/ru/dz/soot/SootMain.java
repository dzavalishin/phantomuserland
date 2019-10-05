package ru.dz.soot;

import java.io.File;
import java.io.IOException;
import java.nio.file.FileVisitResult;
import java.nio.file.FileVisitor;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.logging.Logger;

import ru.dz.plc.PlcMain;
import ru.dz.plc.compiler.ClassMap;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomField;
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
 * Java to Phantom bytecode converter. Uses plc code generation backend.
 * 
 * @author dz
 *
 */
public class SootMain {
	private static int errors = 0;
	private static int warnings = 0;

	static Logger log = Logger.getLogger(SootMain.class.getName());
	static ClassMap classes = ClassMap.get_map();
	private static Set<String> classesToDo = new HashSet<String>();
	private static Set<String> classesToSkip = new HashSet<String>();
	private static String sourceClassPath;



	/**
	 * @param args
	 * @throws PlcException 
	 * @throws IOException 
	 */
	public static void main(String[] args) throws PlcException, IOException {
		String phantomClassPath = "test/pc";

		PlcMain.addClassFileSearchParh(new File(phantomClassPath));
		PlcMain.addClassFileSearchParh(new File("../../plib/bin"));
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
		{
			doClass("test.toPhantom.SootTestClass");

			doClass("test.toPhantom.ArrayAccess");
			doClass("test.toPhantom.D"); // TODO must be compiled before class that uses it

			doClass("test.toPhantom.IPhantomPrinter");
			doClass("test.toPhantom.AllRun");

			doClass("test.toPhantom.ArrayAssigns");
			doClass("test.toPhantom.ArraySimple1");
			doClass("test.toPhantom.Assigns");
			doClass("test.toPhantom.Arrays");
			doClass("test.toPhantom.Strings");
			doClass("test.toPhantom.Loops");


			//doClass("java.lang.");

			doClass("java.lang.UnsatisfiedLinkError");
			doClass("java.lang.NoSuchMethodError");
			doClass("java.lang.AbstractMethodError");
			doClass("java.lang.NoSuchFieldError");
			doClass("java.lang.VerifyError");
			doClass("java.lang.NoClassDefFoundError");
			doClass("java.lang.IllegalAccessError");
			doClass("java.lang.ClassFormatError"); 
			doClass("java.lang.ClassCircularityError"); 
			doClass("java.lang.ThreadDeath"); 
			doClass("java.lang.UnknownError"); 
			doClass("java.lang.StackOverflowError"); 
			doClass("java.lang.OutOfMemoryError"); 
			doClass("java.lang.InternalError"); 
			doClass("java.lang.InstantiationError"); 
			doClass("java.lang.NegativeArraySizeException"); 
			doClass("java.lang.ArrayIndexOutOfBoundsException"); 
			doClass("java.lang.IllegalMonitorStateException"); 
			doClass("java.lang.ClassCastException"); 
			doClass("java.lang.ArrayStoreException");
			doClass("java.lang.ArithmeticException");

			//doClass("java.lang.AbstractStringBuilder");
			//doClass("java.lang.StringBuilder");
		}
		else
		{
			for( String a : args )
			{
				if(a.charAt(0) == '-')
				{
					if( a.charAt(1) == 'c' )
					{
						setSourceClassPath(a.substring(2));
						continue;
					}

					if( a.charAt(1) == 'C' )
					{
						String dir = a.substring(2);
						//say("-C "+dir);
						//Scene.v().setSootClassPath(dir);
						setSourceClassPath(dir);
						queueAllClasses(dir);
						continue;
					}

					if( a.charAt(1) == 'X' )
					{
						String cn = a.substring(2);
						classesToSkip.add(cn);
						continue;
					}

					if( a.charAt(1) == '?' )
					{
						System.out.println(
								"-C<dir> - process all .class files in directory (sets -c dir also)\n"+
										"-c<java-class-path-list> - set directory to load .class files from\n"+
										"-o<phantom-class-out-dir>\n"+
										"-X<class.name - skip (do not convert) class"+
										"-I - ignored (plc compat)"
								);
						continue;
					}

					PlcMain.processFlag(a);
					continue;
				}
				doClass(a);
			}
		}
		/*
		for( String iClass : classesToDo )
		{
			String cName = iClass.replace('/', '.');
			say("Process queued class "+cName);
			doClass(cName);
		}
		 */
		while( !classesToDo.isEmpty() )
		{
			String cName = classesToDo.iterator().next();

			classesToDo.remove(cName);

			cName = cName.replace('/', '.');
			cName = cName.replace('\\', '.');

			say("Process queued class "+cName);
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
		classes.preprocess();
		classes.codegen();
	}


	private static void setSourceClassPath(String dir) {
		sourceClassPath = dir;
		Scene.v().setSootClassPath(dir);
	}


	private static void queueAllClasses(String dir) {
		Path start = java.nio.file.Paths.get(dir);
		FileVisitor<Path> visitor = new FileVisitor<Path>() {

			@Override
			public FileVisitResult preVisitDirectory(Path dir, BasicFileAttributes attrs) throws IOException {
				//System.out.println("dir pre "+dir);
				return FileVisitResult.CONTINUE;
			}

			@Override
			public FileVisitResult visitFile(Path file, BasicFileAttributes attrs) throws IOException {
				//System.out.println("visit "+file);

				if(!attrs.isRegularFile())
				{
					//System.out.println("not file "+file);
					return FileVisitResult.CONTINUE;
				}

				addPath(file);
				return FileVisitResult.CONTINUE;
			}

			@Override
			public FileVisitResult visitFileFailed(Path file, IOException exc) throws IOException {
				System.err.println("scan failed "+file+" "+exc);
				return FileVisitResult.CONTINUE;
			}

			@Override
			public FileVisitResult postVisitDirectory(Path dir, IOException exc) throws IOException {
				//System.out.println("dir post "+dir);
				return FileVisitResult.CONTINUE;
			}

		};

		try {
			say("walk "+start);
			Files.walkFileTree(start, visitor);
		} catch (IOException e) {
			System.err.println("Unable to scan "+start);
			e.printStackTrace();
		}

	}

	// Add path to queue to be compiled
	protected static void addPath(Path file) 
	{	
		Path start = java.nio.file.Paths.get(sourceClassPath);

		//say("abs = "+file);

		//Path relative = file.relativize(start);
		Path relative = start.relativize(file);

		//say("relative = "+relative);

		String className = relative.toString();

		className = className.replaceAll("/", ".");
		className = className.replaceAll("\\\\", "."); // This means replace ONE backslash with .

		if( !className.endsWith(".class") )
			return;

		// remove .class suffix
		className = className.substring(0, className.length()-6);

		//say("add "+className);

		classesToDo.add(className);

	}


	private static void doClass(String cn) throws PlcException, IOException
	{
		if( classesToSkip.contains(cn))
		{
			say("Skipping "+cn);
			return;			
		}
		
		try {

			SootClass c = Scene.v().loadClassAndSupport(cn);
			//Scene.v().loadNecessaryClasses();

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

			ParseState				ps = new ParseState();

			PhantomClass pc = new PhantomClass(convertClassName(c.getName()));

			ps.set_class(pc);
			
			Chain<SootField> fields = c.getFields();
			for( SootField f : fields )
			{
				PhantomType type = SootExpressionTranslator.convertType(f.getType());		
				say("add field '"+f.getName()+"' type '"+type+"' to "+pc.getName());
				PhantomField pf = pc.addField(f.getName(), type);
				pf.setPublic(!f.isPrivate());
			}

			pc.generateGettersSetters(ps);

			List<SootMethod> mlist = c.getMethods();

			for( SootMethod m : mlist )
			{
				SootMethodTranslator mt = new SootMethodTranslator(m,pc, ps);
				//doMethod(m);
				mt.process();
			}

			//{				pc.preprocess( new ParseState(pc) );			}
			classes.add(pc);

			//classes.listMethods();
			//say("Generate Phantom code for "+pc.getName());
			//classes.codegen();
		} catch(PlcException e)
		{
			say("Failed to compile "+cn+": "+e);
		}
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

	public static void say(String string) {
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
