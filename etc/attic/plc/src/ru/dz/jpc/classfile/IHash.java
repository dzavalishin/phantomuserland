//  IHash.java -- interface hash table construction

package ru.dz.jpc.classfile;

import java.io.*;
import java.util.*;

public class IHash {		// method hash table for use at runtime

    public int mask;		// hash mask value
    public Field hlist[];	// interface hash list

static final int MINLOAD = 8;	// minimum acceptable loading with collisions
static final int MINSIZE = 512;	// min acceptable table size with collisions

private static boolean inTranslator = false; // Assume we aren't in the translator

    /* When we're running in the toba translator, we want to look up
     * class data by going from a file name, searching in specific areas.
     * When not running in the translator, we want to get the class that's
     * currently loaded in this execution, if available. */
    public static boolean
    setInTranslator (boolean nv) {
       boolean ov = inTranslator;
       inTranslator = nv;
       return ov;
    }

//  mark(c) -- flag interface methods in ClassData.imtable method table

public static void mark(ClassLoader loader,
                        ClassData curr)
    throws ClassNotFoundException, IOException
{
    for (ClassData c = curr; c != null; c = c.superclass) {
	for (int i = 0; i < c.interfaces.length; i++) {
	    int[] hlist = hashlist(loader, c.interfaces[i]);
	    for (int j = 0; j < hlist.length; j++) {
		int h = hlist[j];
		for (int k = 0; k < curr.imtable.length; k++) {
		    if (curr.imtable[k].hashcode == h) {
			curr.imtable[k].access |= ClassData.FLG_INTERFACE;
                    }
                }
	    }
	}
    }
}



//  hashlist(ClassRef) -- return array of hashcodes of interface class's methods

private static int[] hashlist(ClassLoader loader,
                              ClassRef cr)
    throws ClassNotFoundException, IOException
{
    /* NB: This used to cache results based on the interface name.  The
     * interface name is not a unique identifier---we also need the loader
     * that generated it---so we don't cache anymore. */

    // Don't call ClassFile directly to read the classfile; that may not be
    // where we find this class.  Rely on Class.forName to find the class,
    // and the hash table kept by ClassData to get the data.
    ClassData c;

    /* If we're running inside the translator, we want to look up things
     * by our ClassFile support.  If not, we want to search through the
     * normal Class.forName interface, to link with our versions. */
    if (inTranslator) {
        ClassFile cf = ClassFile.find (cr.name);
        c = ClassData.forStream (null, cf, false);
    } else {
        if (! cr.isResolved (loader)) {
            throw new InternalError ("ClassRef for " + cr.name + " unresolved in loader " + loader);
        }
        /* NB: This only works with the Toba runtime system, because forClass
         * invokes a native function. */
        try {
            c = ClassData.forClass (cr.getRefClass (loader));
        } catch (UnsatisfiedLinkError e) {
            throw new InternalError ("ClassData conversion from class unsupported in this RTS");
        }
    }

    Vector v = new Vector();

    for (int i = 0; i < c.interfaces.length; i++) {	// recursive implements
	int[] hh = hashlist(loader, c.interfaces[i]);
	for (int j = 0; j < hh.length; j++) {
	    v.addElement(new Integer(hh[j]));
        }
    }

    for (int i = 0; i < c.methods.length; i++) {	// present class methods
	v.addElement(new Integer(c.methods[i].hashcode));
    }

    int h [] = new int[v.size()]; // convert to array
    for (int i = 0; i < v.size(); i++) {
    	h[i] = ((Integer) v.elementAt(i)).intValue();
    }

    return h;						// return array
}



//  new IHash(ClassData c) -- create hash table for interface searching
//
//  table is based on methods marked by FLG_INTERFACE in c.imtable

public IHash(ClassData c) {

    Vector mlist = impls(c);

    // determine hashtable length and mask
    int len = 1;
    while (len < mlist.size())
    	len *= 2;
    while (collisions(mlist, len) > 0
        && (len < MINLOAD * mlist.size() || len < MINSIZE))
	    len *= 2;
    mask = len - 1;

    // fill hash vector
    Vector v = new Vector(len);
    for (int i = 0; i < len; i++)
    	v.addElement(null);
    for (int i = 0; i < mlist.size(); i++) {
    	Field m = (Field) mlist.elementAt(i);
	int j = m.hashcode & mask;
	while (j < v.size() && v.elementAt(j) != null)
	    j++;
	if (j < v.size())
	    v.setElementAt(m, j);
	else
	    v.addElement(m);
    }

    // ensure list is terminated, and turn into an array
    if (v.elementAt(v.size() - 1) != null)
    	v.addElement(null);
    hlist = new Field[v.size()];
    v.copyInto(hlist);

    // verify that no two elements have identical hashcodes
    // shouldn't ever happen in real life...
    for (int i = 0; i < hlist.length; i++) {
	for (int j = i + 1; j < hlist.length; j++) {
	    Field f1 = hlist[i];
	    Field f2 = hlist[j];
	    if (f1 != null && f2 != null && f1.hashcode == f2.hashcode) {
	    	System.err.println("incredibly bad luck: hashcodes are equal");
		System.err.println(
		    "   " + hlist[i].hashcode + " " + hlist[i].name);
		System.err.println(
		    "   " + hlist[j].hashcode + " " + hlist[j].name);
	    	System.err.println("this is not going to work...");
	    }
	}
    }
}



//  impls(c) -- build vector of interface methods implemented by class c

private static Vector impls(ClassData c)
{
    Vector v = new Vector();
    for (int i = 0; i < c.imtable.length; i++) {
	if ((c.imtable[i].access & ClassData.FLG_INTERFACE) != 0) {
	    v.addElement(c.imtable[i]);
        }
    }
    return v;
}



//  collisions(mlist, n) -- count collisions for table of size n (a power of 2)

private static int collisions(Vector mlist, int n)
{
    BitSet hit = new BitSet(n);
    int mask = n - 1;
    int ncoll = 0;

    for (int i = 0; i < mlist.size(); i++) {
	Field m = (Field) mlist.elementAt(i);
	if (hit.get(m.hashcode & mask))
	    ncoll++;
	else
	    hit.set(m.hashcode & mask);
    }
    return ncoll;
}



} // class IHash
