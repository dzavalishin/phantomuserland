//  HFile.java -- methods for writing .h files

package ru.dz.jpc.tanslator;

import ru.dz.jpc.classfile.*;
import java.io.*;

@Deprecated
class HFile {



static private int hvnum;		// hidden variable number



//  write(d, c) -- write header info for class c on stream d.

static void write(PrintWriter d, ClassData c)
{
    Field m, f;
    boolean hascv;

    // header file protection
    d.println();
    d.println("#ifndef h_" + c.cname);
    d.println("#define h_" + c.cname);

    // define an "init_classname" macro, possibly having no effect if unneeded
    d.println();
    d.print("#define init_" + c.cname + "() ");
    if (c.getmethod("<clinit>", false) == null)
	d.println("(void)0");
    else
	d.println("if (cl_" + c.cname + ".C.needinit) initclass(&cl_" + 
	    c.cname + ".C)");

    // function declarations
    d.println();
    for (int i = 0; i < c.methods.length; i++) {
	m = c.methods[i];
	d.println(Repr.rettype(m.signature) + "\t" + m.cname +
	    "(" + argtypes(m) + ");");
    }

    // method table
    d.println();
    d.println("struct mt_" + c.cname + " {");
    for (int i = 0; i < c.imtable.length; i++) {
	m = c.imtable[i];
	d.println("    struct {TobaMethodInvokeType itype;" +
	    Repr.rettype(m.signature) + "(*f)(" + argtypes(m) +
	    ");\n\tconst Char *name_chars;int name_len;" + 
	    "const Char *sig_chars;int sig_len;" +
	    "\n\tint localp;int access;int classfilePos;Class *xlist;} " + m.cname + ";");
    };
    d.println("};");

    // are there any class variables?
    hascv = false;
    for (int i = 0; i < c.fields.length; i++) {
	if ((c.fields[i].access & ClassData.ACC_STATIC) != 0) {
	    hascv = true;
	    break;
	}
    }

    // declare class variable struct, if needed
    if (hascv) {
	d.println();
	d.println("struct cv_" + c.cname + " {");
	for (int i = 0; i < c.cvtable.length; i++) {
	    f = c.cvtable[i];
	    declare(d, f, f.cname);
	}
	d.println("};");
    }

    // class struct
    d.println();
    d.println("extern struct cl_" + c.cname + " {");
    d.println("    struct class C;");
    d.println("    struct mt_" + c.cname + " M;");
    if (hascv)
	d.println("    struct cv_" + c.cname + " V;");
    d.println("} cl_" + c.cname + ";");

    // instance struct
    d.println();
    d.println("struct in_" + c.cname + " {");
    d.println("    struct cl_" + c.cname + " *class;");
    d.println("    struct monitor *monitor;");
    hvnum = 0;				// init counter of shadowed vars
    ivfield(d, c);			// declare instance variable fields
    d.println("};");

    // tail
    d.println();
    d.println("#endif /* h_" + c.cname + " */");
}


//  ivfield(d, c) -- declare instance variables for c and ancestors.

static private void ivfield(PrintWriter d, ClassData c)
{
    if (c == null)
	return;
    for (int i = 0; i < c.ivtable.length; i++) {
	Field f = c.ivtable[i];
	if (c.visiblevars.get(f.name) == f)
	    declare(d, f, f.cname);
	else
	    declare(d, f, Names.hashvar(++hvnum + "shadowed"));
    }
}



//  declare(d, v, name) -- generate a declaration for a variable.

static private void declare(PrintWriter d, Field v, String name)
{
    String prefix = "    ";
    if ((v.access & ClassData.ACC_VOLATILE) != 0)
	prefix = "    volatile ";
    d.println(prefix + Repr.ctype(v.signature.charAt(0)) + " " + name + ";");
}



//  argtypes(m) -- return string of C arg types for Java method m.

private static String argtypes(Field m)
{
    String s = m.signature;
    StringBuffer b = new StringBuffer();

    if ((m.access & ClassData.ACC_STATIC) == 0)
	b.append("Object,");			// "self"

    int i = 1;
loop:
    while (true) {
	switch (s.charAt(i)) {
	    case ')':  break loop;
	    case 'B':  b.append("Byte,");     break;
	    case 'C':  b.append("Char,");     break;
	    case 'D':  b.append("Double,");   break;
	    case 'F':  b.append("Float,");    break;
	    case 'I':  b.append("Int,");      break;
	    case 'J':  b.append("Long,");     break;
	    case 'S':  b.append("Short,");    break;
	    case 'Z':  b.append("Boolean,");  break;
	    case 'L':				// object
		b.append("Object,");
		i = s.indexOf(';', i);
		break;
	    case '[':				// array
		b.append("Object,");
		while (s.charAt(i) == '[')
		    i++;
		if (s.charAt(i) == 'L')
		    i = s.indexOf(';', i);
		break;
	}
	i++;
    }

    if (b.length() == 0)
	return "void";
    else {
	b.setLength(b.length() - 1);		// trim final ","
	return b.toString();
    }
}



} // class HFile
