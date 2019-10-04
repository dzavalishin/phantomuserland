//  Trans.java -- Toba classfile translator
//
//  usage:  xtoba [-dxxxx] [-m mainclass] [-P dir] [-r] class...
//
//  Usually, the translator is called by the "toba" script.
//  If called directly, the "-dxxxx" command option sets debugging
//  flags.  Debugging output is written on standard output.
//  -m is used to specify a mainclass to generate
//  -r recursively pull in dependencies
//

package ru.dz.jpc.tanslator;

import ru.dz.jpc.classfile.*;
import java.io.*;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;
@Deprecated
class Trans {



static final String PROGNAME = "Toba";
static final String usagestring =
    "usage: xtoba [-dxxxx] [-m mainclass] [-P dir] [-r] class...";

static final int cBufferSize = 8192;
static final int hBufferSize = 8192;
static final int mBufferSize = 8192;
static final int classBufferSize = 8192;
static final int peekBufferSize = 1024;

static String fileheader = "/*  created by " + PROGNAME + "  */";

static boolean echo_names;	// echo file names when compiling?



//  command line debugging options
static final char dbgAll = 'a';	// everything
static final char dbgGeneral = 'g';	// generally useful stuff

static final char dbgConstants = 'k';	// dump constant table of initial class
static final char dbgTrace = 't';	// trace major actions by translator
static final char dbgTemp = 'x';	// for temporary debugging hacks
static final char dbgCode = 'c';	// dump analyzed bytecode as a unit
static final char dbgInstrs = 'i';	// list instrs with generated code

private static boolean dbgflag[] = new boolean[256];

private static boolean autodep;		// pull in all dependencies
private static String mainclass;	// name of main class
private static String packagedir;	// destination package directory 

private static boolean needmain = true;	// need to generate main
private static Hashtable genlist;	// list of classes we generated
private static Stack classWorkList;     // list of classes we still need to visit
private static String tobapath;		// toba.class.path property value

private static boolean retobaapi = false; // re-translate stuff we found in an API

private static boolean okToInstallPkgClasses = true; // install class files when building packages?

//static final Logger log = Logger.getLogger("ru.dz.jpc.Trans");
//static { log.setLevel(Level.INFO); }

//  main(args) -- main program.

public static void main(String args[]) 
{
	Logger log = Logger.getLogger("ru.dz.jpc.Trans");
    int i;

    log.info("Phantom Java compiler start");
    
    options(args);			// process options

    tobapath = System.getProperty("toba.class.path");
    genlist = new Hashtable();
    classWorkList = new Stack ();

    // Set things so we look up interfaces in the right place.
    IHash.setInTranslator (true);

    // locate main from user provided name
    if (needmain && (mainclass != null)) {
        doname(mainclass);
        if (needmain)
            abort("static void main(String[]) not found in " + mainclass);
    }

    // process all command line classes
    for (i = 0; i < args.length; i++) {
	if (args[i] != null)
	    doname(args[i]);		// process other arguments
    }
}



//  options(args[]) -- record options and null them in arglist.
//
//  very simple -- really need a "getopt" to do this right

private static void options(String args[])
{
    int i;
    String s;
    char c;
    int nopts = 0;

    for (i = 0; i < args.length; i++) {
	s = args[i];
	if (s.length() < 2 || s.charAt(0) != '-')
	    continue;
	nopts++;
	args[i] = null;
	c = s.charAt(1);
	switch (c) {
	    case 'd':           // set debug flags
		setdbg(s.substring(2));
		break;
            case 'm':           // name main class
                i++;
                if (i >= args.length)
                    abort(usagestring);
                mainclass = args[i];
                args[i] = null;
                break;
            case 'M':           // Don't emit a main procedure
                i++;
                needmain = false;
                break;
	    case 'P':           // name package directory
                i++;
                if (i >= args.length)
                    abort(usagestring);
                packagedir = args[i];
                needmain = false;
                args[i] = null;
                break;
            case 'p':
                okToInstallPkgClasses = false;
                break;
            case 'r':           // track down dependences
                autodep = true;
                echo_names = true;
                break;
            case 'R':           // retobaapi
                retobaapi = true;
                break;
	    default:
		abort("Unrecognized option " + s + "\n" + usagestring);
	}
    }

    ClassFile.trace = debugging(dbgTrace);

    if (args.length - nopts > 1)
	echo_names = true;
}



//  setdbg(s) -- set debugging options based on flag characters

static void setdbg(String s)
{
    int i;

    // set selected flags
    for (i = 0; i < s.length(); i++)
	dbgflag[(int)s.charAt(i)] = true;

    // any options at all imply general debugging
    dbgflag[(int)dbgGeneral] = true;

    // option 'a' implies all flags
    if (dbgflag[(int)dbgAll])
	for (i = 0; i < dbgflag.length; i++)
	    dbgflag[i] = true;
}



//  debugging(c) -- check if a particular debugging char is selected

static boolean debugging(char c)
{
    return dbgflag[(int)c];
}



//  doname(name) -- process one class, given by file or class name.

private static void doname(String name)
{
	Logger log = Logger.getLogger("ru.dz.jpc.Trans");
	
    classWorkList.push (name);

    while (! classWorkList.empty ()) {

        name = (String) classWorkList.pop ();


        
        // stop if we've already done this one
        // XXX can we do this here?  can differences between name and
        // k.name give us headaches?
        if (genlist.get(name) != null) {
            continue;
        }

        ClassFile cf = null;
        ClassData k = null;

        try {				// load class
            /* Force a find of this by its name; we may not want a version we
             * would get by Class.forName.  Don't remember the ClassData
             * structure: during bootstrapping, that consumes too much memory. */
            cf = ClassFile.find(name);
            k = ClassData.forStream (null, cf, false);
            Supers.load(k);			// load superclasses and method tables
        } catch (ClassNotFoundException e) {
            abort("cannot load class " + name + ": " +
                  e.getMessage() + " not found");
        } catch (FileNotFoundException e) {
            abort("cannot load file " + e.getMessage());
        } catch (ClassFormatError e) {
            abort("cannot load " + name + ": " + e.getMessage());
        } catch (IOException e) {
            abort("I/O error loading " + name + ": " + e);
        }

        // stop if we've already done this one
        if (genlist.get(k.name) != null) {
            continue;
        }
        
        genlist.put(k.name, k.name);

        // try to generate a main method if none has been generated yet
        if (needmain) {
            genmain(k);
        }

        // unless told to do so, don't generate C if it's from the toba path -
        // we generated previously
//        System.out.println ("cf.path = " + ((null != cf.path) ? cf.path : "<null>") + " ; tobapath = " + tobapath);

        if ((! retobaapi) &&
            (cf.path != null) &&
            cf.path.equals(tobapath)) {
            continue;
        }

        if (echo_names) {
            System.err.println(name + ":");
        }

        /* If we're building a package and we're supposed to do the classfile
         * installation, do it.  Classes go in the "classes" subdirectory
         * of the package directory. */
        if ((packagedir != null) && okToInstallPkgClasses) {
            try {
                ClassInstall.install(name, k.name, packagedir + File.separatorChar + "classes");
            } catch(IOException e) {
                abort("Could not copy " + k.name + " to " + packagedir);
            } catch(ClassNotFoundException e) {
                abort(name + " disappeared!");
            }
        }

        PrintWriter hstream = oopen(k, ".h", hBufferSize);
        HFile.write(hstream, k);		// write .h file
        hstream.close();

        PrintWriter cstream = oopen(k, ".c", cBufferSize);
        if (debugging(dbgConstants)) {
            Constant.dump(cstream, k.constants);
        }
        CFile.write(cstream, k);		// write .c file
        cstream.close();

        // recursively generate for all dependencies
        if (autodep) {
            for (int i = 1; i < k.constants.length; i++) {
                Constant c = k.constants[i];
                if (c != null && c.tag == Constant.CLASS) {
                    ClassRef cr = (ClassRef)c.value;
                    String depclass = Names.baseclass(cr.name);
                    if (depclass != null) {
                        classWorkList.push (depclass);
                    }
                }
            }
        }
    }
        
    return;
}



//  oopen(class, suffix, bufsize) -- open output file for generated code.

static PrintWriter oopen(ClassData k, String suffix, int bufsize)
{
    String filename = k.fname + suffix;
    File f = new File(filename);

    if (foreign(f))
	abort(filename + ": contents unrecognized; will not overwrite");

    if (f.exists() && !f.canWrite())
	abort(filename + ": cannot write");

    try {
	PrintWriter d = new PrintWriter(
	    new BufferedOutputStream(new FileOutputStream(f), bufsize));
	d.println("/*  " + f.getName() + " -- from Java class " 
	    + k.name + "  */");
	d.println(fileheader);
	return d;
    } catch(Exception e) {
	abort(filename + ": cannot open for output");
    }
    return null;
}


// genmain(class) -- try to generate a C main

static void genmain(ClassData k)
{
    Field mainmethod = null;

    // check if there is a public static void main(String[])
    for (int i = 0; i < k.methods.length; i++) {
        Field m = k.methods[i];
        if (m.name.equals("main") &&
            m.signature.equals("([Ljava/lang/String;)V") &&
            m.access == (ClassData.ACC_PUBLIC | ClassData.ACC_STATIC)) {
             mainmethod = m;
             break;
        }
    }  

    if (mainmethod == null)
        return;        

    PrintWriter mstream = oopen(k, "_main.c", mBufferSize);

    mstream.println("#include \"toba.h\"");
    mstream.println("#include \"" + k.fname + ".h\"");
    mstream.println("\n\n");
    mstream.println("\n");
    mstream.println("int main(int argc, char *argv[])\t\t/* main entry */");
    mstream.println("{");
    mstream.println("\treturn start(&cl_" + k.cname + ".C, " +
        mainmethod.cname + ", argc, argv);");
    mstream.println("}");     
    mstream.close();

    // we generated main, don't need one anymore
    needmain = false;
}


//  foreign(f) -- is file f a "foreign" file (not to be overwritten)?

private static boolean foreign(File f)
{
    if (!f.exists() || f.length() == 0)
	return false;
    try {
        BufferedReader rd = new BufferedReader (new FileReader(f));
	rd.readLine();
	String s = rd.readLine();
	rd.close();
	return !s.equals(fileheader);
    } catch(Exception e) {
	return false;
    }
}



//  abort(s) -- abort run, with a message.

static void abort(String s)
{
    System.err.print("   ");
    System.err.println(s);
    System.exit(1);
}



} // class Trans
