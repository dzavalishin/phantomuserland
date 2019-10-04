//  FieldRef.java -- a Variable, Method, or InterfaceMethod ref in a Constant table

package ru.dz.jpc.classfile;

public abstract class FieldRef {
    public ClassRef cl;
    public String name;
    public String signature;
    
    private Field     refField;         // the refered-to field

    // The constructor called when parsing class files
    FieldRef(ClassRef clref, String n, String s) {
	cl = clref;
	name = n;
	signature = s;
    }
    

    abstract public void resolveWith(ClassData cdata);

    public boolean isResolved() {
	if (refField == null)
	    return false;
	else
	    return true;
    }

    protected void resolveTo(Field f) {
	refField = f;
    }

    public Field getField() {
	if (!isResolved()) {
	    throw new NoClassDefFoundError("Reference to " + this + " is unresolved.");
	}
	return refField;
    }

    public String toString() {
	return signature + " " + cl + "." + name;
    }
};
