//  Supers.java -- operations involving superclasses

package ru.dz.jpc.tanslator;

import ru.dz.jpc.classfile.*;
import java.io.*;

@Deprecated
public class Supers {


private static ClassData previous;	// last class loaded

//  load(k) -- load and link superclasses of ClassData k
//
//  sets k.superclass, and all method tables (and those of all ancestors)
//
//  It is assumed that previously seen classes can be reused,
//  allowing caching.

public static void load(ClassData k)
    throws ClassNotFoundException, IOException
{
    ClassData kp;
    String sname;

    sname = k.supername;		// name of superclass

    if (k.superclass == null && sname != null) {

    	// there is a superclass and it has not been linked in

	for (kp = previous; kp != null; kp = kp.superclass)
	    if (kp.name.equals(sname))
		break;

	if (kp != null)
	    k.superclass = kp;		// found in existing chain
	else {
	    // not found; find and load superclass from file
            ClassFile cf = ClassFile.find (sname);
	    k.superclass = ClassData.forStream (null, cf, false);
//            System.out.println (sname + " from " + cf.dir);

            /* Resulting class has to have the right name. */
            if (! sname.equals (k.superclass.name)) {
                throw new ClassNotFoundException (sname);
            }
	    load(k.superclass);		// load superclass's superclasses
	}
    }

    k.state = ClassData.RES_SUPERCLASSES;

    k.buildTables();			// fill in method table
    IHash.mark(null, k);			// mark interfaces
    previous = k;			// remember class just loaded
}
    




} // class Supers
