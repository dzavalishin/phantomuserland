//  MethodRef.java -- a Method or InterfaceMeth ref in a Constant table

package ru.dz.jpc.classfile;

public class MethodRef extends FieldRef {
    public MethodRef(ClassRef cl, String n, String s) { super(cl, n, s); }

    // Locate the method that this reference refers
    // to in the given ClassData
    protected Field findMethod(ClassData cdata) {
        Field rm;
        Field [] mt;
        int i;

        /* This method is wrong in exactly the same way as VariableRef's
         * findVariable.  It may return the wrong reference if a class
         * has a static and instance method of the same name (due to
         * overloading). */

        /* Give static methods priority; one of them has to come first.... */
        mt = cdata.smtable;
        i = mt.length;
        while (0 <= --i) {
	    if ((name == mt[i].name) && 
		(signature == mt[i].signature)) {
		return mt[i];
            }
	}

        /* If no static method found, look through the dynamic table. */
	mt = cdata.imtable;
        i = mt.length;
        while (0 <= --i) {
	    if ((name == mt[i].name) && 
		(signature == mt[i].signature)) {
		return mt[i];
            }
	}

        /* Blow chow. */
	throw new NoSuchMethodError(cdata.name + "." + name + signature);
    }

    public void resolveWith(ClassData cdata) {
	resolveTo(findMethod(cdata));
    }
};
