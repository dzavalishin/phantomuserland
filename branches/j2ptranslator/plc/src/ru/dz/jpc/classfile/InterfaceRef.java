//  InterfaceRef.java -- an InterfaceMethod ref in a Constant table

package ru.dz.jpc.classfile;

public class InterfaceRef extends MethodRef {
    public InterfaceRef(ClassRef cl, String n, String s) { super(cl, n, s); }
    
    // Find the Field in cdata that cooresponds to the
    // referenced interface
    public Field findInterface(ClassData cdata) {
	try {
	    return findMethod(cdata);
	} catch (NoSuchMethodError e) {
	    throw new IncompatibleClassChangeError();
	}
    }
};
