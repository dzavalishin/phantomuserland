//  ClassFile.java -- locate a Java .class file
//
//   Note:  these methods just open files; they don't check class names.

package ru.dz.jpc.classfile;

import java.io.*;
import java.util.*;
//import sun.tools.zip.*;
import java.util.zip.*;

public class ClassFile extends DataInputStream {



	static final int classBufferSize = 8192;	// class file buffer size

	public static boolean trace = false;		// tracing flag



	// instance variables

	public File file;			// file handle (for name, age, etc.)
	public String path;			// path found on 
	public String dir;          // directory found in

	public ZipFile zipfile;		// opened Zip File



	// new ClassFile(InputStream input) -- open a class file given the input stream

	private ClassFile(InputStream input)
	{
		super(new BufferedInputStream(input, classBufferSize));
	}


	//  new ClassFile(f) -- open a class file given a file handle.

	public ClassFile(File f)
	throws FileNotFoundException
	{
		this(new FileInputStream(f));
		file = f;
		path = null;
	}


	//  new ClassFile(filename) -- open a class file given the exact file name.

	public ClassFile(String filename)
	throws FileNotFoundException
	{
		this(new File(filename));
	}


	//  find(classname) -- find a class given its name, checking usual places.

	public static ClassFile find(String classname)
	throws ClassNotFoundException
	{
		ClassFile f;

		// check for name ending in .class, which indicates a file name
		if (classname.endsWith (".class")) {
			try {
				if (trace)
					System.out.println("  filename load: " + classname);
				f = new ClassFile(classname);
				f.dir = classname;
				return f;
			} catch (FileNotFoundException e) {
				throw new ClassNotFoundException(classname);
			}
		}

		if (trace)
			System.out.println("  searching for class " + classname);

		// check for illegal class name that might map into a legal file name
		if (classname.indexOf(File.separator) >= 0)
			throw new ClassNotFoundException(classname);

		f = null;
		String classPath;

		/* Look in TOBAPATH first.  TOBAPATH contains the names of the package
		 * root areas; tack a "/classes" on to each package directory to find
		 * the class hierarchy for the package. */
		classPath = System.getProperty ("toba.class.path");
		
		if (null == classPath) classPath = "."; 
		
		if (null != classPath) {
			try {
				f = findInPath (classPath, classname, "/classes");
			} catch (ClassNotFoundException e) {
				/* do nothing; keep f null */
			}
		}
		if (null != f) {
			return f;
		}
		/* No good with that; try CLASSPATH now. */
		classPath = System.getProperty ("java.class.path");
		if (null != classPath) {
			try {
				f = findInPath (classPath, classname, "");
			} catch (ClassNotFoundException e) {
			}
		}

		// finally strip off package name and try current directory
		/* Don't do that; the spec says that you look in areas in CLASSPATH
		 * only.  There's no implicit addition of . as a fallback. */

		if (null == f) {
			throw new ClassNotFoundException (classname);
		}
		return f;

	}


	// close() -- close the class file

	public void close() 
	throws IOException
	{
		IOException te;

		te = null;
		try {
			super.close();
		} catch (IOException e) {
			te = e;
		}
		if (zipfile != null) {
			try {
				zipfile.close();
			} catch (IOException e) {
				if (null == te) {
					te = e;
				}
			}
			zipfile = null;
		}
		if (null != te) {
			throw te;
		}
	}

	//------------------------------- private ------------------------------------



	// find(path, classname) -- find a class in a search path
	private static
	ClassFile
	findInPath (String classpath,   // Path (set of directories) to search
			String classname,   // Name of class
			String dsuffix)     // Suffix to add to each directory
	throws ClassNotFoundException
	{
		if (null == classpath) {
			return null;
		}

		String filepath;
		int i = 0;
		int j;

		/* Get the name, in its hierarchy, of the class file that should
		 * hold the data for the named class. */
		filepath = classname.replace('.', File.separatorChar) + ".class";

		/* classpath is a string of paths wherein class files might
		 * be found.  The paths are separated by File.pathSeparator characters.
		 * Look in each path for the given file.  Create a new copy that's
		 * terminated with the path separator, to simplify the loop control
		 * logic. */
		String nclasspath;
		nclasspath = classpath + File.pathSeparator;
		while (0 <= (j = nclasspath.indexOf(File.pathSeparator, i))) {
			String dname;           // Directory we're going to look in

			dname = nclasspath.substring(i, j) + dsuffix;
			if (0 == dname.length()) {
				dname = null;			// if empty, use current dir
			}

			ClassFile cf = open(dname, filepath);
			if (cf != null) {
				/* Save the unmodified class path so we can tell whether
				 * we found this on tobapath or classpath. */
				cf.path = classpath;
				return cf;
			}
			i = j + 1;
		}

		if (trace) {
			System.out.println("    CLASS NOT FOUND: " + classname);
		}
		throw new ClassNotFoundException(classname);	// no luck
	}

	// open(dir, file) -- open a class file, possibly in a .zip file
	//
	// If dir is null, the current directory is used.

	private static ClassFile open(String dir, String file)
	{
		if (trace)
			System.out.println("      checking for " + file + " in " + dir);

		// If dir exists and is a plain file, treat it as a zipfile
		if (dir != null && (new File(dir)).isFile() ) {
			ZipFile zf = null;

			// zipfiles seem to use '/' always
			file = file.replace(File.separatorChar, '/');
			try {
				File f = new File(dir);
				zf = new ZipFile(f);
				ZipEntry entry = zf.getEntry(file);
				if (entry != null) {
					if (trace)
						System.out.println("      found " + 
								dir + File.separator + file);
					ClassFile ret = new ClassFile(zf.getInputStream(entry));
					ret.zipfile = zf;
					ret.file = f;
					ret.dir = dir;
					return ret;
				}
				zf.close();
			} catch(IOException e1) {
				try {
					if (zf != null)
						zf.close();
				} catch(IOException e2) {
				}
			}
		} else {
			File f = new File(dir, file);
			if (f.exists()) {
				if (trace)
					System.out.println("      found " + f.getPath());
				try {
					ClassFile cf = new ClassFile(f);
					cf.dir = dir + "/" + file;
					return cf;
				} catch(FileNotFoundException e) {
				}
			}
		}
		return null;
	}



} // class ClassFile
