//  ClassInstall.java -- for copying class files into a package directory

package ru.dz.jpc.tophantom;

import ru.dz.jpc.classfile.ClassFile;
import java.io.*;

public class ClassInstall {



public static void mkdir(String name) 
    throws IOException
{
    name = name.replace('/', File.separatorChar);
    File f = new File(name);
    if (!f.isDirectory() && !f.mkdirs())
	throw new IOException("Could not make directory " + name);
}

public static void copy(DataInputStream src, String destname) 
       throws IOException
{
    int len;
    byte[] buf = new byte[4096];
    boolean okToIgnoreEOFE;

    okToIgnoreEOFE = false;
    try {
        destname = destname.replace('/', File.separatorChar);
        FileOutputStream dest = new FileOutputStream(destname);
        okToIgnoreEOFE = true;
        while (true) {
            len = src.read(buf);
            if (len <= 0)
                break;
            dest.write(buf, 0, len);
        }
        okToIgnoreEOFE = false;
        dest.close ();
    } catch (EOFException e) {
        /* Under Irix with jdk 1.1.5, extracting certain classes from
         * rt.jar seems to throw the following exception _during_the_write_
         * of the uncompressed class file.  Checking one or two of the
         * classes indicates that the write actually does complete, so
         * we'll ignore that exception. */
        if (! (okToIgnoreEOFE &&
               e.toString().equals ("java.io.EOFException: Unexpected end of ZLIB input stream"))) {
            throw e;
        }
    }
}

public static void install(String findname, String classname, String target)
    throws IOException, ClassNotFoundException
{

    // Make sure the target exists; then make sure it's a directory.
    // Some bad assumptions here about what we're given as target.
    if (0 < target.length ()) {
        mkdir (target);
    }
    target += File.separatorChar;

    // Get a handle for reading the class file
    ClassFile f = ClassFile.find(findname);

    // turn class name into a filename
    String filename = classname;
    if (! filename.endsWith (".class")) {
        filename = filename.replace('.', '/') + ".class";
    }

    // copy it into place
    int index = filename.lastIndexOf('/');
    if (index > 0) {
	String dir = filename.substring(0, index);
	mkdir(target + dir);
    }
    copy(f, target + filename);
    f.close();
}


} // class ClassInstall
