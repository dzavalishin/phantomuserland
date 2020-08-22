//  Field.java -- Methods and Variables

package ru.dz.jpc.classfile;

import java.io.*;

public class Field {		// field or method data

    public String name;		// declared name
    public String cname;	// name used in C
    public String signature;	// signature
    public int hashcode;	// hash code for method searching
    public int access;		// access flags
    public Attribute attributes[];	// attributes
    public Field next;		// link to other methods with same name

    // not set by constructor, but later by Supers.methTable():

    public int overrides;	// if >0, overrides ancestor method this 
                                // many levels up
    public int tableslot;	// method/var table slot; -1 if unset
    public int classfilePos;    // index of field in classfile
    public Object tableObj;     // what is in this table slot


private void
finishField ()
{
    hashcode = Names.hashinterface(name, signature);
}

//  new Field(stream, ctab) -- load field or method info from class file
Field(DataInputStream d, Constant c[])
    throws IOException
{
    access = d.readUnsignedShort();
    name = (String)c[d.readUnsignedShort()].value;
    signature = (String)c[d.readUnsignedShort()].value;
    attributes = new Attribute[d.readUnsignedShort()];
    for (int i = 0; i < attributes.length; i++) {
	attributes[i] = new Attribute(d, c);
    }
    tableslot = -1;
    finishField ();
}

/* Characters that denote field types in parameter/return descriptors */
public static final char FT_byte      = 'B';
public static final char FT_char      = 'C';
public static final char FT_double    = 'D';
public static final char FT_float     = 'F';
public static final char FT_int       = 'I';
public static final char FT_long      = 'J';
public static final char FT_object    = 'L';
public static final char FT_short     = 'S';
public static final char FT_boolean   = 'Z';
public static final char FT_array     = '[';
public static final char FT_void      = 'V';

/** Return the size, in bytes, of the native representation of the field.
  * Field had better name a variable, not a method.
  * @returns size in bytes of field value
  */
public int
getVarFieldSize ()
{
    switch (signature.charAt (0)) {
	case FT_void:     return 0;
	case FT_byte:     return 1;
	case FT_char:     return 2;
	case FT_double:   return 8;
	case FT_float:    return 4;
	case FT_int:      return 4;
	case FT_long:     return 8;
	case FT_short:    return 2;
	case FT_boolean:  return 1;
        case FT_object:   return 4;
	default:
            throw new InternalError ("getVarFieldSize: Unrecognized field type " + signature.charAt (0));
    }
}

public String
toString ()
{
    return name + " (" + signature + ") @ " + tableslot + "/" + overrides;
}

public boolean
equals (Field that)
{
    // Hopefully, we'll hit this most often, but I don't trust that.
    if (that == this) {
        return true;
    }
    // Close enough if all these match. */
    return (name.equals (that.name) &&
            signature.equals (that.signature) &&
            (access == that.access) &&
            (tableslot == that.tableslot));
    
}

public boolean
isInArray (Field fa [])
{
    boolean islocal;
    int j;

    /* Determine whether we're a member of this array */
    islocal = false;
    j = fa.length - 1;
    while ((! islocal) &&
           (0 <= j)) {
        islocal = this.equals (fa [j]);
        --j;
    }
    return islocal;
}

} // class Field
