//  Names.java -- methods dealing with transformations of names

package ru.dz.jpc.classfile;

import java.io.*;
import java.util.*;
import java.util.zip.*;

public class Names {



static private final int MaxIdLength = 31;
static private final int MinIdUsed = 5;
static private final int HashLength = 5;
static private final int MaxSigLength = MaxIdLength - HashLength - MinIdUsed - 2;

static private final String alnums =
	    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static private final int hashchoices = alnums.length();

static private final int HashMultClass = 43;
static private final int HashMultName = 41;
static private final int HashMultSignature = 37;



//  classfile(name) -- choose file name (excluding extension) for a class.

public static String classfile(String name)
{
    return name.replace('.', '_');
}



//  hashclass(name) -- hash a class name into C form
//
//  For a class name, this usually just means changing . to _.
//  An illegal C character, including $, forces use of a hashed name.

public static String hashclass(String name)
{
    name = name.replace('.','_');
    if (idstring(name))
	return name;

    StringBuffer b = new StringBuffer(MaxIdLength);
    String h = hashname(name, "", "", b);

    if (b.length() == 0 || Character.isDigit(b.charAt(0)))
	b.insert(0, 'C');		// prefix with C if necessary

    if (b.length() + 1 + HashLength > MaxIdLength)
	b.setLength(MaxIdLength - HashLength - 1);

    return b + "_" + h;
}



//  classref(signature) -- return class or pseudo-class matching signature
//
//  (Assumes any leading '[' characters have been stripped off.)

public static String classref(String signature)
{
    switch (signature.charAt(0)) {
	case 'B':  return "cl_byte";
	case 'C':  return "cl_char";
	case 'D':  return "cl_double";
	case 'F':  return "cl_float";
	case 'I':  return "cl_int";
	case 'J':  return "cl_long";
	case 'S':  return "cl_short";
	case 'Z':  return "cl_boolean";
	default:   return "cl_" +
	    hashclass(signature.substring(1, signature.length() - 1)) + ".C";
    }
}



//  hashvar(name) -- map variable name to C form
//
//  A name is left unchanged if it is a legal C identifier
//  and does not match a reserved macro or typedef name.

public static String hashvar(String name)
{
    if (idstring(name) && name.length() <= MaxIdLength && !reserved(name))
	return name;			// OK as is 

    StringBuffer b = new StringBuffer(MaxIdLength);
    String h = hashname(name, "", "", b);

    if (b.length() == 0 || Character.isDigit(b.charAt(0)))
	b.insert(0, 'V');		// prefix with V if necessary

    if (b.length() + 1 + HashLength > MaxIdLength)
	b.setLength(MaxIdLength - HashLength - 1);

    return b + "_" + h;
}



//  hashinterface(name, signature) -- map interface to hashed integer.

public static int hashinterface(String name, String sig)
{
    int i;
    char c;
    int sum = 0;

    for (i = 0; i < name.length(); i++) {
	c = name.charAt(i);
	sum = HashMultName * sum + (int)c;
    }
    for (i = 0; i < sig.length(); i++) {
	c = sig.charAt(i);
	sum = HashMultSignature * sum + (int)c;
    }
    return sum;
}



//  hashmethod(name, classname, signature) -- map method name to C form

public static String hashmethod(String name, String classname, 
				String signature)
{
    String csig = csignature(signature);
    StringBuffer b = new StringBuffer(MaxIdLength);
    String h = hashname(name, classname, signature, b);

    if (b.length() == 0 || Character.isDigit(b.charAt(0)))
	b.insert(0, 'M');		// prefix with M if necessary

    if (b.length() + 1 + csig.length() + 1 + HashLength > MaxIdLength)
	b.setLength(MaxIdLength - HashLength - 1 - csig.length() - 1);

    return b + "_" + csig + "_" + h;
}



//  hashname(sbuf, name, classname, signature) -- return hash string for name
//
//  The hash is a string of alphanumeric characters (as defined by C) based
//  on the name, classname, and signature of a field.  sbuf is a string buffer
//  in which the [C] alphanumeric characters of the name are placed.

static private String hashname
    (String name, String classnm, String sig, StringBuffer b)
{
    StringBuffer h = new StringBuffer(HashLength);
    long sum = 0;
    char c;
    int i;

    // class name: contributes to hash sum
    for (i = 0; i < classnm.length(); i++) {
	c = classnm.charAt(i);
	if (c == '.')
	    c = '/';			// compatible with unfixed names
	sum = HashMultClass * sum + (int)c;
    }

    // field name: accumulate hash value and save "good" characters
    for (i = 0; i < name.length(); i++) {
	c = name.charAt(i);
	sum = HashMultName * sum + (int)c;
	if (idchar(c))
	    b.append(c);
    }

    // signature: contributes to hash sum
    for (i = 0; i < sig.length(); i++) {
	c = sig.charAt(i);
	sum = HashMultSignature * sum + (int)c;
    }

    // compute hash characters
    sum = sum & 0xFFFFFFFFL;		// compatible with C version
    for (i = 0; i < HashLength; i++) {
	h.append(alnums.charAt((int)(sum % hashchoices)));
	sum /= hashchoices;
    }

    return h.toString();
}



//  csignature(signature) -- calculate C signature of a Java method
//
//  The C signature is a string of zero or more characters.  Each argument
//  type contributes one character to the signature.  Additionally, an array
//  type argument adds one prefixing "a" for each dimension of the array.
//  The length is limited to MaxSigLength characters, and the method result
//  type does not contribute to the signature.
//
//  Object types are represented by the first character of the final
//  component of the name.  For example, a java.lang.String argument
//  is represented by the letter `S'.  Primitive types are represented
//  by these codes:
//	b	byte
//	c 	character
//	d	double
//	f	float
//	i	int
//	l	long
//	s	short
//	z	boolean
//
//  C signatures created by these rules are intended to aid readability.
//  They are ambiguous and cannot be used as sole identification.
//  For example, a signature of "aS" probably means that a method takes
//  a single argument that is an array of java.lang.String.  It could,
//  however, take an "arrow" argument followed by a "Sam" argument.

static String csignature(String sig)
{
    StringBuffer b = new StringBuffer(MaxSigLength);
    boolean inclass = false;

    for (int i = 0; i < sig.length(); i++) {
	char c = sig.charAt(i);
	if (inclass) {
	    if (c == '/' || c == '.')
		b.setCharAt(b.length() - 1, sig.charAt(i + 1));
	    else if (c == ';')	
		inclass = false;
	} else switch (c) {
	    case '(':  break;
	    case 'L':  b.append(sig.charAt(i + 1)); inclass = true; break;
	    case 'B':  b.append('b');  break;
	    case 'C':  b.append('c');  break;
	    case 'D':  b.append('d');  break;
	    case 'F':  b.append('f');  break;
	    case 'I':  b.append('i');  break;
	    case 'J':  b.append('l');  break;
	    case 'S':  b.append('s');  break;
	    case 'Z':  b.append('z');  break;
	    case '[':  b.append('a');  break;
	    case ')':  return b.toString();
	    default:   b.append('x');  break;	// error
	}
	if (b.length() >= MaxSigLength && !inclass)
	    break;
    }
    return b.toString();
}



//  idstring(name) -- is name a legal C identifier?

static private boolean idstring(String name)
{
    char c = name.charAt(0);
    if (c == '_' || Character.isDigit(c))
	return false;			// starts with '_' or digit
    for (int i = 0; i < name.length(); i++)
	if (! idchar(name.charAt(i)))
	    return false;		// illegal character
    return true;
}



//  idchar(c) -- is character c usable as part of a C identifier?

static private boolean charflags[] = new boolean[256];
static {
    for (int i = 0; i < alnums.length(); i++)
	charflags[(int)alnums.charAt(i)] = true;
    charflags[(int)'_'] = true;
}

static private boolean idchar(char c)
{
    int n = (int) c;
    return (n < charflags.length) && (charflags[n]);
}



//  reserved(name) -- is name reserved for use by Java?
//
//  Certain names must be modified because they are reserved by C,
//  defined in #include files, or used by Toba.
//
//  This list must reflect all macro and names included or defined by
//  the generated code, as well as all C reserved words.

static private Hashtable nametable;
static private String[] namelist = {
    "setjmp", "longjmp", "jmp_buf",
    "sigsetjmp", "siglongjmp", "sigjmp_buf",
    "unix", "sun", 
    "asm", "auto", "break", "case", "char", "const", "continue",
    "default", "do", "double", "else", "enum", "entry", "extern",
    "float", "for", "fortran", "goto", "if", "int", "long", "register",
    "return", "short", "signed", "sizeof", "static", "struct", "switch",
    "typedef", "union", "unsigned", "void", "volatile", "while",
    "Boolean", "Char", "Byte", "Short", "Int", "Long", "Float", "Double",
    "Void", "Object", "Class", "UInt", "ULong",
    "CARRAY", "GOTO", "GOBACK", "YIELD", "YIELD_FN", "RETTO", "NOTHREADS",
    "HASHMASK", "TOBA_CLASSSTRUCT_VERSION", "PROLOGUE",
    "TobaMethodInvokeType", "TMIT_undefined", "TMIT_native_code",
    "TMIT_interpreter", "TMIT_uninstalled_jit", "TMIT_abstract",
};
static {
    nametable = new Hashtable();
    for (int i = 0; i < namelist.length; i++)
	nametable.put(namelist[i], "");
}

static private boolean reserved(String s)
{
    char a, b;

    if (s.length() < 2)
	return false;		// too short to be a problem

    a = s.charAt(0);
    b = s.charAt(1);
    if (a == '_' && (b == '_' || Character.isUpperCase(b)))
	return true;		// matches C implementation space

    return nametable.containsKey(s);	// check table
}


//  baseclass(s) -- return substring identifying base class of s.
//
//  If s is an array of primitives, baseclass() returns null.  Otherwise,
//  s is a classname or an array class signature ("[...[Lclassname;"),
//  and baseclass() returns the classname.

public static String baseclass(String s)
{
    int i;

    if (s.charAt(0) != '[')
        return s;
    for (i = 1; s.charAt(i) == '['; i++)
        ;
    if (s.charAt(i) == 'L')
        return s.substring(i + 1, s.length() - 1);
    else
        return null;                            // baseclass is primitive
} 

    /** This is the implementation of ClassLoader.getResourceAsInputStream.
      * Handling the file/ziparchive variations of this and all the exceptions
      * is too hard in C.  It's very much like t.c.ClassFile.open, but
      * that's got too many dependencies in it to abstract away for code
      * reuse, and I'm not going to redesign the interface now. */
    private static InputStream
    toba_getSRAS (String name)
    {
        int i;
        int j;
        String classPath;
        
        classPath = System.getProperty ("java.class.path");
        if (null == classPath) {
            classPath = ".";
        }
        classPath += File.pathSeparator;
        i = 0;
        while (0 <= (j = classPath.indexOf (File.pathSeparator, i))) {
            String dir = classPath.substring (i, j);
            i = j + 1;
            if (0 == dir.length()) {
                dir = null;
            }

            // If dir exists and is a plain file, treat it as a zipfile
            if (dir != null && (new File(dir)).isFile() ) {
                ZipFile zf = null;

                // zipfiles seem to use '/' always
                String zfe = name.replace(File.separatorChar, '/');
                try {
                    File f = new File(dir);
                    zf = new ZipFile(f);
                    ZipEntry entry = zf.getEntry(zfe);
                    if (entry != null) {
                        return zf.getInputStream (entry);
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
                File f = new File(dir, name);
                if (f.exists()) {
                    try {
                        return new FileInputStream (f);
                    } catch (Exception e) {
                    }
                }
            }
        }
        return null;
    }

} // class Names
