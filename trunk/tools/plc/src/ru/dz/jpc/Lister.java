// $Id: Lister.java,v 1.6 1999/03/09 01:57:28 pab Exp $
// list classes from default location
// Also determine transitive closure of class dependencies

package ru.dz.jpc;

import ru.dz.jpc.classfile.*;
import ru.dz.jpc.tanslator.*;

import java.io.*;
import java.util.*;
import java.util.zip.*;

public class Lister {

private static boolean trace = false;

private static void printname(String name) 
{
    try {
	// strip off ".class" and change file separators to '.'
	name = name.substring(0, name.length() - 6);
	name = name.replace(File.separatorChar, '.');
	name = name.replace('/', '.');
	System.out.println(name);
    } catch(StringIndexOutOfBoundsException e) {
    }
}

// recursively descend directory adding entries to hash table
private static void recursivelist(String prefix, String dir) {
    File f = new File(dir, prefix);
    String[] files = f.list();
    if (files == null)
	return;
    for (int i = 0; i < files.length; i++) {
	String name = prefix + files[i];
	File file = new File(dir, name);
	if (f.isDirectory())
	    recursivelist(name + File.separator, dir);
	if (name.endsWith(".class")) {
	    printname(name);
	}
    }
}

// print out all system classes from the same location as java/lang/String
public static void listSystemClasses() {
    String aSystemClassFile = "java/lang/String.class";
    aSystemClassFile.replace('/', File.separatorChar);

    // walk the class path until we see a java directory
    String classpath = System.getProperty("java.class.path");
    int i, j;
    for (i = 0; (j = classpath.indexOf(File.pathSeparator, i)) >= 0; 
	    i = j + 1) {
	String dir = classpath.substring(i, j);
	if (dir.endsWith(".zip") ||
	    dir.endsWith(".jar")) {
	    // look through .zip
	    try {
		ZipFile zf = new ZipFile(dir);
		ZipEntry zfe = zf.getEntry(aSystemClassFile);
		if (zfe == null) {
		    zf.close();
		    continue;
		}

		// we found a system class, this is the one we want
		Enumeration files = zf.entries();
		while (files.hasMoreElements()) {
		    zfe = (ZipEntry)files.nextElement();
		    String name = zfe.getName();
		    if (name.endsWith(".class"))
			printname(name);
		}
		zf.close();
		return;
	    } catch(IOException e) {
	    }
	} else {
	    // look through directory
	    File f = new File(dir + File.separatorChar + aSystemClassFile);
	    if (!f.exists())        
		continue;

	    // we found a system class, this is the one we want
	    recursivelist("", dir);
	    return;
	}
    }

    System.err.println("No System Classes Found (is CLASSPATH set?)!");
    System.exit(1);
}

public static void
abort (String s) {
    System.err.println (s);
    System.exit (-1);
}

/** Given an array of class names, determine the transitive closure of
  * all classes they depend on.  Write the complete dependency list to
  * stdout. */
public static void
closeClassList (String cnames [],
                int cnind)
{
    Hashtable genlist; // list of classes we generated
    Stack worklist;  // list of classes we need to generate

    ru.dz.jpc.classfile.IHash.setInTranslator (true);

    genlist = new Hashtable ();
    worklist = new Stack ();
    if (cnind < cnames.length) {
        while (cnind < cnames.length) {
            worklist.push (cnames [cnind]);
            ++cnind;
        }
    } else {
        BufferedReader stdin = new BufferedReader (new InputStreamReader (System.in));
        while (true) {
            String cn;

            cn = null;
            try {
                cn = stdin.readLine ();
            } catch (Exception e) {
            }
            if (null == cn) {
                break;
            }
            worklist.push (cn);
        }
    }

    while (! worklist.empty ()) {
        String name = (String) worklist.pop ();

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
             * would get by Class.forName.  Use a null ClassLoader, to pretend
             * we're using the system one.  Don't remember the ClassData
             * structure: during bootstrapping, that consumes too much memory. */
            cf = ClassFile.find(name);
            k = ClassData.forStream (null, cf, false);
//            Supers.load(k);			// load superclasses and method tables
        } catch (ClassNotFoundException e) {
            System.err.println("cannot load class " + name + ": " +
                  e.getMessage() + " not found");
        } catch (FileNotFoundException e) {
            System.err.println("cannot load file " + e.getMessage());
        } catch (IOException e) {
            System.err.println("I/O error loading " + name + ": " + e);
        }
        
        if (null == cf) {
            // Put it on the list anyway.
            genlist.put(name, name);
            continue;
        }

        // stop if we've already done this one
        if (genlist.get(k.name) != null) {
            continue;
        }

//        System.out.println ("< " + k.name + " from " + cf.dir);
//        System.out.println ("< " + k.name);
        genlist.put(k.name, k.name);

        for (int i = 1; i < k.constants.length; i++) {
            Constant c = k.constants[i];
            if (c != null && c.tag == Constant.CLASS) {
                ClassRef cr = (ClassRef)c.value;
                String depclass = Names.baseclass(cr.name);
                if (null != depclass) {
                    if (null == genlist.get(depclass)) {
                        worklist.push (depclass);
                    }
                    if (trace) {
                        System.out.println ("# " + name + " needs " + depclass);
                    }
                }
            }
        }
    }

    Enumeration elt = genlist.elements ();
    while (elt.hasMoreElements ()) {
        String sn = (String) elt.nextElement ();
        System.out.println (sn);
    }
}
        


public static void main(String[] args) {
    int aind = 0;
    if ((aind < args.length) && args[aind].equals ("-trace")) {
        trace = true;
	aind++;
    }
    if ((aind < args.length) && args[aind].equals ("-close")) {
        closeClassList (args, aind+1);
    } else {
        listSystemClasses();
    }
}



} // class Lister
