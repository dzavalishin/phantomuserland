//  CFile.java -- general procs for writing .c files, excluding method code

package ru.dz.jpc.tophantom;

import ru.dz.jpc.classfile.*;
//import ru.dz.plc.compiler.ClassDefinitionNode;
import ru.dz.plc.compiler.ClassMap;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.util.PlcException;

import java.io.*;
import java.util.*;

class CFile {



static private Hashtable declared;	// set of class structs declared

static private StringBuffer strpool;	// string character pool
static private int strnums[];		// indices of used strings
static private int strlens[];		// lengths of those string
static private int strcount;		// number of used strings

static private int nsupers;		// number of superclasses incl self
static private int ninters;		// number of interfaces implemented
static private int nothers;		// number of "other" classes referenced



//  write(d, c) -- write header info for class c on stream d.

static void write(PrintWriter d, ClassData c, ClassMap classes) throws PlcException
{
	//PhantomClass me = new PhantomClass(".java."+c.fname);
	PhantomClass me = new PhantomClass("."+c.fname); 
	classes.add(me);
	
	ParseState				ps = new ParseState();	
	ps.set_class(me);
	
    declared = new Hashtable();		// clear list of classes

    // include general definitions, present class, ancestors, their interfaces
    d.println();
    d.println("#include \"toba.h\"");
    for (ClassData t = c; t != null; t = t.superclass) {
	include(d, t.name);
	for (int i = 0; i < t.interfaces.length; i++)
	    include(d, t.interfaces[i].name);
    }

    // always include these two classes (needed by generated code):
    include(d, "java.lang.String");
    include(d, "java.lang.Class");
    
    // include declarations for other referenced classes
    for (int i = 1; i < c.constants.length; i++) {
	Constant k = c.constants[i];
	if (k != null && k.tag == Constant.CLASS) {
	    ClassRef cr = (ClassRef)k.value;
	    String s = Names.baseclass(cr.name);
	    if (s != null)
		include(d, s);
	}
    }

    declared = new Hashtable();		// reset list of classes
    supers(d, c);			// generate superclass list
    inters(d, c);			// generate interfaces list
    others(d, c);			// generate others list

    d.println();
    d.println("extern const Char ch_" + c.cname + "[];");	// declare string pool
    d.println("extern const void *st_" + c.cname + "[];");	// declare string ptrs

    for (int i = 0; i < c.methods.length; i++) {
	Field m = c.methods[i];
        /* This is really static, but we have to declare it extern
         * to get by the Irix C compiler which won't allow forward
         * decls of arrays of unknown size.  We can't create a Method
         * structure now, because the variable use flags will be
         * wrong unless we've done some of the later setup. */
	d.println("extern Class xt_" + m.cname + "[];");// declare exceptn lists
    }

    hashgen(d, c);			// generate hash table

    clname(d, c);			// generate class name string
    cltables(d, c);                     // generate variable static tables
    clstruct(d, c);			// generate class structure

    gfloats(d, c.constants);		// generate floating constants

    // initialize bookkeeping for string pool
    strnums = new int[c.constants.length];	// init constant index array
    strlens = new int[c.constants.length];	// init string lengths array
    strpool = new StringBuffer(c.name);		// init pool with class name
    strlens[0] = c.name.length();		// record it as first entry
    strcount = 1;				// and count it

    // generate method code
    for (int i = 0; i < c.methods.length; i++) {
	Method m = new Method(c, c.methods[i]);
	MethGen.minfo(d, m, me);			// always gen exception list
	if ((m.fl.access & ClassData.ACC_ABSTRACT) == 0) {
	    // generate body code if not native
	    if ((m.fl.access & ClassData.ACC_NATIVE) == 0) {
		try {
		    MethGen.mgen(d, m, me, ps);
		} catch (Error e) {
		    Trans.abort("method " + m.fl.name + ": " + e.getMessage());
		}
	    }
	    // if synchronized, generate wrapper function
	    if ((m.fl.access & ClassData.ACC_SYNCHRONIZED) != 0)
		MethGen.syncwrap(d, m);
	}
    }

    // dump string pool
    strdump(d, c);
}



//  include(d, name) -- generate #include for a class, if not already generated

static private void include(PrintWriter d, String name)
{
    name = Names.classfile(name);
    if (!declared.containsKey(name)) {
	d.println("#include \"" + name + ".h\"");
	declared.put(name, "");
    }
}



// supers(d, c) -- write list of superclasses.

static private void supers(PrintWriter d, ClassData c)
{
    nsupers = 0;
    d.println();
    d.println("static const Class supers[] = {");
    for (; c != null; c = c.superclass) {
	d.println("    &cl_" + c.cname + ".C,");
	declared.put(c.cname, "");
	nsupers++;
    }
    d.println("};");
}


// inters(d, c) -- write list of interfaces implemented.

static private void inters(PrintWriter d, ClassData c)
{
    ninters = 0;
    d.println();
    d.println("static const Class inters[] = {");
    for (; c != null; c = c.superclass) {
	for (int j = 0; j < c.interfaces.length; j++) {
	    String s = Names.hashclass(c.interfaces[j].name);
	    d.println("    &cl_" + s + ".C,");
	    declared.put(s, "");
	    ninters++;
	}
    }
    if (ninters == 0)
	d.println("    0");
    d.println("};");
}


// others(d, c) -- write list of other classes referenced.

static private void others(PrintWriter d, ClassData c)
{
    nothers = 0;
    d.println();
    d.println("static const Class others[] = {");
    for (int i = 1; i < c.constants.length; i++) {
	Constant k = c.constants[i];
	if (k != null && k.tag == Constant.CLASS) {
	    ClassRef cr = (ClassRef)k.value;
	    String s = Names.baseclass(cr.name);
	    if (s != null) {
		s = Names.hashclass(s);
		if (! declared.containsKey(s)) {
		    d.println("    &cl_" + s + ".C,");
		    nothers++;
		}
	    }
	}
    }
    if (nothers == 0)
	d.println("    0");
    d.println("};");
}



//  hashgen(d, c) -- generate hash table for finding interface methods

static private int hcols;
static private final int HColMax = 75;

static private void hashgen(PrintWriter d, ClassData c)
{
    IHash h = new IHash(c);
    d.println();
    d.println("#define HASHMASK 0x" + Integer.toHexString(h.mask));
    for (int i = 0; i < h.hlist.length; i++) {
    	Field m = h.hlist[i];
	if (m != null) {
	    d.println("/*  " + Integer.toHexString(i) + ".  " + 
		Integer.toHexString(m.hashcode) + "  (" + 
	    	Integer.toHexString(m.hashcode & h.mask) +
		")  " + m.name + "  */");
	}
    }
    d.print("static const struct ihash htable[" + h.hlist.length + "] = {");
    hcols = HColMax;
    for (int i = 0; i < h.hlist.length; i++) {
    	Field m = h.hlist[i];
	if (m == null)
	    hashprint(d, " 0, 0,");
	else
	    hashprint(d, " " + m.hashcode + ", &cl_" + c.cname + ".M." +
	    	m.cname + ",");
    }
    d.println("\n};");
}

static private void hashprint(PrintWriter d, String s)
{
    if (hcols + s.length() > HColMax) {
    	d.print("\n   ");
	hcols = 3;
    }
    d.print(s);
    hcols += s.length();
}

/** Emit the name and signature unicode char arrays for a given set of
  * fields, tagging each definition with something that indicates whether
  * it's static/instance method/field. */
static private void
emitNamesSigs (PrintWriter d,   // Where to write data
               String tag,      // Identifying tag
               Field [] farr)   // Array of fields
{
    for (int i = 0; i < farr.length; i++) {
        d.println ("static const Char nm" + tag + "_" + i + "[] = {");
        Repr.emitCharData (d, farr [i].name);
        d.println ("};");
        d.println ("static const Char sg" + tag + "_" + i + "[] = {");
        Repr.emitCharData (d, farr [i].signature);
        d.println ("};");
    }
    return;
}
               

//  clname(d, c) -- generate class name constant.
//
//  Some class names are needed during initialization, but the string
//  constant generated here is later replaced by an interned version.

static private void clname(PrintWriter d, ClassData c)
{
    d.println();
    d.println("static const CARRAY(" + c.name.length() + 
        ") nmchars = {&acl_char, 0, " + c.name.length() + ", 0,");
    Repr.emitCharData (d, c.name);
    d.println("};");
    d.println("static struct in_java_lang_String classname =");
    d.println("    { &cl_java_lang_String, 0, (Object)&nmchars, 0, " +
        c.name.length() + " };");
}



//  cltables(d, c) -- generate variable static tables.

static private void cltables(PrintWriter d, ClassData c)
{

        /* We have to match these structures from toba.h:
struct vt_generic {
   int offset;            If nonzero, is offset of field from instance start
   void *addr;            If nonnull, is address of class variable
   const Char * name_chars; Name of the field
   int name_len;            Length of field name
   const Char * sig_chars;  Signature of the field
   int sig_len;             Length of field signature
   int localp;            Nonzero iff field is declared in this class (not superclass)
   int access;            Access flags for field
   int classfilePos;            Index of variable in original classfile
};

struct mt_generic {
   TobaMethodInvokeType itype;  Type/source of method
   Void (*f) ();                Function entry point
   const Char * name_chars;     Name of the method
   int name_len;                Length of method name
   const Char * sig_chars;      Signature of the method
   int sig_len;                 Length of method signature
   int localp;                  Nonzero iff method is declared in this class (not superclass)
   int access;                  Access flags for method
   int classfilePos;            Index of method in original classfile
   Class *xlist;                Exception list, if local method
};
         */

    /* Have to generate Java-format strings for instance and static fields,
     * and instance and static methods, names and signatures both. */
    emitNamesSigs (d, "cv", c.cvtable);
    emitNamesSigs (d, "iv", c.ivtable);
    emitNamesSigs (d, "sm", c.smtable);
    emitNamesSigs (d, "im", c.imtable);

    // Class variable table
    d.println();
    d.println("static struct vt_generic cv_table[] = {");
    for (int i = 0; i < c.cvtable.length; i++) {
	Field f = c.cvtable[i];
	d.println("    {0," + 
		  "&cl_" + c.cname + ".V." + f.cname + 
                  ",(const Char *)&nmcv_" + i + "," + f.name.length () +
                  ",(const Char *)&sgcv_" + i + "," + f.signature.length () +
                  (f.isInArray (c.fields) ? ",1" : ",0") +
                  ",0x" + Integer.toHexString (f.access) +
                  "," + f.classfilePos + "}, ");
    }

    if (c.cvtable.length == 0) {
	d.println("    {0}");
    }
    d.println("};");

    // Instance variable table
    d.println();
    // Define the offsetof macro
    d.println("#ifndef offsetof");
    d.println("#define offsetof(s,m) ((int)&(((s *)0))->m)");
    d.println("#endif");

    d.println("static struct vt_generic iv_table[] = {");
    for (int i = 0; i < c.ivtable.length; i++) {
	Field f = c.ivtable[i];
	d.println("    { offsetof(struct in_" +c.cname+", " +f.cname+"), 0" +
		  ",(const Char *)&nmiv_" + i + "," + f.name.length () +
                  ",(const Char *)&sgiv_" + i + "," + f.signature.length () +
                  (f.isInArray (c.fields) ? ",1" : ",0") +
                  ",0x" + Integer.toHexString (f.access) +
                  "," + f.classfilePos + "}, ");
    }
    if (c.ivtable.length == 0)
	d.println("    {0}");
    d.println("};");
    d.println("#undef offsetof");

    // Static method table
    d.println();
    d.println("static struct mt_generic sm_table[] = {");
    for (int i = 0; i < c.smtable.length; i++) {
	Field f = c.smtable[i];
	int localp = f.isInArray (c.methods) ? 1 : 0;
	d.println("    {TMIT_native_code, (Void(*)())" + f.cname +
		  ",\n\t(const Char *)&nmsm_" + i + "," + f.name.length () +
                  ",(const Char *)&sgsm_" + i + "," + f.signature.length () +
                  ",\n\t" + localp + ",0x" + Integer.toHexString (f.access) +
                  "," + f.classfilePos + 
		  "," + (localp == 1 ? ("xt_" + f.cname) : "0") + "},");
    }
    if (c.smtable.length == 0)
	d.println("    {TMIT_undefined}");
    d.println("};");
}



/** Search the class fields, and if any have initializers, create a function
  * that will assign them before we start real execution; this will be
  * needed for reflection. */
static private boolean
emitStaticFinalInit (PrintWriter d, // Where to write data
                     ClassData c) // Information on class
{
    boolean haveSFI;            // Have we started an SFI function?

    haveSFI = false;
    for (int i = 0; i < c.cvtable.length; i++) {
        Field f = c.cvtable [i];
        byte adata [];
        int cind;

        // See if the field has a constant value (initializer)
        adata = Attribute.find (f.attributes, "ConstantValue");
        if (null == adata) {
            continue;
        }
        /* OK, it does.  What's the value of the initializer?  If the
         * exception is thrown below, we probably had an invalid class
         * file. */
        cind = -1;
        try {
            cind = (new DataInputStream (new ByteArrayInputStream (adata))).readShort ();
        } catch (IOException e) {
        }
        if (0 > cind) {
            continue;
        }
        /* OK, we're going to need an SFI.  If we haven't started one, do
         * so now. */
        if (! haveSFI) {
            haveSFI = true;
            d.println ();
            d.println ("static void\ninitStaticFields (void) {");
            d.println ("    extern struct in_java_lang_String *intern_string(struct in_java_lang_String *str);");
        }
        /* Name of the field we're going to assign. */
        String fname = "cl_" + c.cname + ".V." + f.cname;

        /* Do the initialization within a separate block so we can define
         * local variables to assign in the init. */
        d.println ("   {");
        Constant ival = c.constants [cind];
        switch (ival.tag) {
            case Constant.LONG: // Just assign value
	        String s;
	        long v = ((Long)ival.value).longValue();
	        if (v > (long)Integer.MIN_VALUE && v < (long)Integer.MAX_VALUE) 
		{
		    s = ival.value.toString();
	        } else {
		    // now we must use "ANSI C" to construct a possibly 
		    // "long long" val
		    int lh = (int)(v >>> 32);
		    int rh = (int)v;
		    s = "((((Long)0x" + Integer.toHexString(lh) +
		        "u)	 << 32) | ((Long)0x" + Integer.toHexString(rh) +
			"u))";
	        }
	        d.println ("   " + fname + " = " + s + ";");
	        break;
            case Constant.FLOAT: { // Union integral representation with float
                float fv;
                
                fv = ((Float)ival.value).floatValue ();
                d.println ("   union fconst _fv = {0x" +
                           Integer.toHexString (Float.floatToIntBits (fv)) +
                           "};");
                d.println ("   " + fname + " = _fv.v;");
                break;
            }
            case Constant.DOUBLE: { // Union integral repr with doublen
                double dv;

                dv = ((Double)ival.value).doubleValue ();
                d.println ("   union dconst _dv = {0x" +
                           Long.toHexString (Double.doubleToLongBits (dv)) +
                           "L};");
                d.println ("   " + fname + " = _dv.v;");
                break;
            }
            case Constant.INTEGER: // Just the value
                d.println ("   " + fname + " = " +
                           ((Integer)ival.value).toString() +
                           ";");
                break;
            case Constant.STRING: {
                /* Create a character array containing the data, then set
                 * up a java.lang.String object pointing to it.  Remember to
                 * intern the string. */
                String sv = (String) ival.value;
                d.println ("    static const CARRAY(" + sv.length () +
                           ") _svchars = {&acl_char, 0, " + sv.length () + ", 0,");
                Repr.emitCharData (d, sv);
                d.println ("    };");
                d.println ("    static struct in_java_lang_String _svjls =");
                d.println ("    { &cl_java_lang_String, 0, (Object) &_svchars, 0, " + sv.length () + "};");
                d.println ("   " + fname + " = intern_string (&_svjls);");
                }
                break;
            default:
                throw new InternalError ("Invalid constant type for static final initializer");
        }
        d.println ("   }");
    }
    if (haveSFI) {
        /* Close off the function if we have one. */
        d.println ("}");
    }
    return haveSFI;
}



//  clstruct(d, c) -- write class struct.
//
//  Must be kept in sync with class struct definition in toba.h.

/* The class structure format number; keep this in sync with toba.h */
static private final int TOBA_CLASSSTRUCT_VERSION = 43;

static private void clstruct(PrintWriter d, ClassData c)
{
    /* Emit the initializer function for static finals (if there is one);
     * remember whether it's there so we can save its address for future
     * invocation. */
    boolean haveSFI = emitStaticFinalInit (d, c);

    // First, generate a static canonical class reference for the class.
    // We don't have a real toba.classfile.ClassRef structure, so do something
    // that looks much the same. */
    d.println();
    d.println("#ifndef h_toba_classfile_ClassRef");
    d.println("extern struct cl_generic cl_toba_classfile_ClassRef;");
    d.println("#endif /* h_toba_classfile_ClassRef */");
    d.println("static struct { /* pseudo in_toba_classfile_ClassRef */");
    d.println("   struct cl_generic *class;");
    d.println("   struct monitor *monitor;");
    d.println("   Object name;");
    d.println("   Object refClass;");
    d.println("   Object loadedRefdClasses;");
    d.println("} inr_" + c.cname + " = {");
    d.println("  (struct cl_generic *)&cl_toba_classfile_ClassRef.C, 0, &classname, &cl_" +
              c.cname + ".C.classclass, 0};");

    d.println();
    d.println("struct cl_" + c.cname + " cl_" + c.cname + " = { {");

    if ((c.access & ClassData.ACC_INTERFACE) != 0)
	d.println("    1, 1,"); // needinit, flags (IS_RESOLVED == 1)
    else
	d.println("    1, 0,");

    d.println("    &classname,");		// class name String
    d.println("    &cl_java_lang_Class.C, 0,");	// class Class instance

    d.println("    sizeof(struct in_" + c.cname + "),");
    d.println("    " + c.imtable.length + ",");
    d.println("    " + c.smtable.length + ",");
    d.println("    " + c.ivtable.length + ",");
    d.println("    " + c.cvtable.length + ",");
    d.println("    " + nsupers + ", supers,");
    d.println("    " + ninters + ", " + c.interfaces.length +
    			", inters, HASHMASK, htable,");
    d.println("    " + nothers + ", others,");
    d.println("    0, 0,");			// arrayclass, elemclass
    d.println("    ch_" + c.cname + ",");	// string pool
    d.println("    st_" + c.cname + ",");	// string list

    methodref(d, c, "<clinit>", false);		// class initializer
    constructor(d, c);				// constructor
    methodref(d, c, "finalize", true);		// finalizer
    d.println("    " + (haveSFI ? "initStaticFields" : "0") + ",");
    d.println("    0,");                        // classloader - 0 since
                                                // system class loader
    d.println("    " + CFile.TOBA_CLASSSTRUCT_VERSION + ","); // class structure version
    d.println("    0x" + Integer.toHexString (c.access) + ","); // class access flags
    d.println("    0,");                        // class data (none to start)
    d.println("    (struct in_toba_classfile_ClassRef *)&inr_" + c.cname + ",");     // canonical class reference structure
    d.println("    iv_table, cv_table,");	// variable tables
    d.println("    sm_table},");		// static method table
    
    methodsigs(d, c);				// instance methods and sigs

    d.println("};");
}


//  methodref(d, c, name, climb) -- find "()V" method and write its name

static private void methodref
    (PrintWriter d, ClassData c, String name, boolean climb)
{
    Field m = c.getmethod(name, climb);
    if (m != null)
	d.println("    " + m.cname + ",");
    else
	d.println("    0,");			// not found
}


//  constructor(c, d) -- generate constructor or exception tosser reference

static private void constructor(PrintWriter d, ClassData c)
{
    if ((c.access & (ClassData.ACC_ABSTRACT | ClassData.ACC_INTERFACE)) != 0) {
	d.println("    throwInstantiationException,");
	return;
    }
    Field m = c.getmethod("<init>", false);
    if (m == null)
	d.println("    throwNoSuchMethodError,");
    else
	d.println("    " + m.cname + ",");
}


//  methodsigs(d, c) -- generate signatures and references for method table.

static private void methodsigs(PrintWriter d, ClassData c)
{
    d.println ("    { /* methodsigs */");
    for (int i = 0; i < c.imtable.length; i++) {
	Field m = c.imtable[i];
            
	if ((m.access & ClassData.ACC_ABSTRACT) != 0)
	    d.print("\t{TMIT_abstract, 0");
	else {
	    String s = String.valueOf(m.hashcode);
	    d.print("\t{TMIT_native_code, " + m.cname);
	}

	int localp = m.isInArray (c.methods) ? 1 : 0;
        d.println(",(const Char *)&nmim_" + i + "," + m.name.length () +
                  ",\n\t(const Char *)&sgim_" + i + "," + m.signature.length() +
		  "," + localp + ",0x" + Integer.toHexString(m.access) + 
                  "," + m.classfilePos + "," +
		  (localp == 1 ? ("xt_" + m.cname) : "0") + "},");
    };
    d.println("    } /* end of methodsigs */");
}

//  gfloats(d, table) -- generated needed floating constants for class.

static void gfloats(PrintWriter d, Constant table[])
{
    d.println();
    for (int i = 1; i < table.length; i++) {
	Constant c = table[i];
	if (c == null || (c.tag != Constant.FLOAT && c.tag != Constant.DOUBLE))
	    continue;
	double v = ((Number)c.value).doubleValue();
	if (Repr.simplefloat(v))
	    continue;			// will generate in-line value
	if (c.tag == Constant.FLOAT)
	    d.println("static union fconst fc" + i + " = { 0x" +
		Integer.toHexString(Float.floatToIntBits((float)v)) + " };");
	else {
	    long b = Double.doubleToLongBits(v);
	    String lh = Integer.toHexString((int)(b >> 32));
	    String rh = Integer.toHexString((int)(b));
	    d.println("static union dconst dc" + i + " = { (ULong)0x" + lh +
		"<<32 | (ULong)(UInt)0x" + rh + " };");
	}
    }
}



//  strref(k) -- generate reference index for constant k, which is a string.

static int strref(Constant k)
{
    int n;

    n = strnums[k.index];		// get index into string pool
    if (n != 0)				// if already allocated
    	return n;			// return index
    
    // append to string pool and create a new slot
    n = strcount++;			// count entry
    strpool.append((String)k.value);	// append characters to pool
    strlens[n] = ((String)k.value).length();	// remember length
    strnums[k.index] = n;		// remember index
    return n;				//   and return it
}

//  strdump(d, c) -- dump string pool on stream d for class c.

static void strdump(PrintWriter d, ClassData c)
{
    String chname = "ch_" + c.cname;

    // generate string pool (a C array of Chars)
    d.println();
    d.println();
    d.println();
    d.print("const Char " + chname + "[] = {  /* string pool */");
    Repr.emitCharData (d, strpool.toString ());
    d.println("};");

    // generate array of pointers to ends of strings
    int base = 0;		/* starting point of current string */
    d.println();
    d.println("const void *st_" + c.cname + "[] = {");
    for (int i = 0; i < strcount; i++) {
	/* generate pointer */
        d.print("    " + chname + "+" + (base + strlens[i]) +
	    ",\t/* " + i + ". ");
	/* in comment, print first 40 chars of each constant */
	for (int j = 0; j < Repr.GCHARS_PER_CMMT && j < strlens[i]; j++) {
	    char ch = strpool.charAt(base + j);
	    if (ch >= ' ' && ch <= '~' && ch != '/' && ch != '?')
	    	d.print(ch);
	    else
	    	d.print("%");	/* use % as stand-in for troublesome chars */
	}
	d.println(" */");
	base += strlens[i];	/* incr base for next string */
    }
    d.println("    0};");	/* terminate table with sentinel */

}



} // class CFile
