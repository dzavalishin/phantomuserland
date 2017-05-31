//  FieldRef.java -- a Field ref in a Constant table

package ru.dz.jpc.classfile;

public class VariableRef extends FieldRef {
    public VariableRef(ClassRef cl, String n, String s) { super(cl, n, s); }

    protected Field
    findVariable (ClassData cdata)
    {
        Field [] vt;
        int i;

        /* This method is fundamentally wrong: it fails to account for cases
         * where fields have the same name and signature, as in hiding
         * a superclass public field, overriding a superclass private field,
         * or obscuring a superclass instance/static variable with a subclass
         * static/instance variable.  What we do to decrease the frequency
         * of screwup is go through the static var table first (because
         * the serialization code hits this bug with a reference to a static
         * field that ends up accessing a parent instance field), and by
         * going from the end of the table down, since that's the order
         * of priority (heading towards superclasses). */
        
        vt = cdata.cvtable;
        i = vt.length;
        while (0 <= --i) {
	    if ((name == vt[i].name) && 
		(signature == vt[i].signature)) {
		return vt[i];
            }
	}

        /* No static field found; try the instance fields */
	vt = cdata.ivtable;
        i = vt.length;
        while (0 <= --i) {
	    if ((name == vt[i].name) && 
		(signature == vt[i].signature)) {
		return vt[i];
            }
	}

        /* No field with this name. */
	throw new NoSuchFieldError(cdata.name + "." + name + "(" + signature + ")");
    }

    public void resolveWith(ClassData cdata) {
	resolveTo(findVariable(cdata));
    }

};
