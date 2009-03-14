//  Dump.java -- Dump class files using Toba's class-reading code.
//
//  Usage:  dump class...
//
//  This program is not a formal part of Toba.  It was built to
//  debug and illustrate the interface to the class-reading code.



package ru.dz.jpc;

import ru.dz.jpc.classfile.*;
import java.io.*;

class Dump {


//  Dump(args) -- main program.

public static void main(String args[]) 
    throws Exception
{
    ClassFile.trace = true;

    for (int i = 0; i < args.length; i++) {
	System.out.println();
	System.out.println();
	System.out.println(args[i] + ":");
	try {
	    doname(args[i]);
	} catch (ClassNotFoundException e) {
	    System.out.println("  CLASS NOT FOUND");
	}
    }
}


//  doname(name) -- process one class, given by file or class name.

private static void doname(String name)
    throws Exception
{
    ClassFile d;
    ClassData k;

    int len = name.length();
    if (len > 6 && name.substring(len - 6).equals(".class"))
	d = new ClassFile(name);	// loading by file name
    else
	d = ClassFile.find(name);	// loading by class name

    System.out.println("  file " + d.file);
    System.out.println("    dir " + d.dir);
    System.out.println("    size " + d.file.length() +
    	", modified " + d.file.lastModified());

    k = ClassData.forStream(d);		// load the class

    printclass(k);			// print class header info

    for (int i = 0; i < k.methods.length; i++) {
    	Method m = new Method(k, k.methods[i]);
    	printmethod(m);			// print method info
    }
}



//  printclass(class) -- print class header info.

private static void printclass(ClassData k)
{
    System.out.println();
    printflags(k.access);
    System.out.println("class " + k.name + " extends " + k.supername);

    for (int i = 0; i < k.interfaces.length; i++)
        System.out.println("implements " + k.interfaces[i]);

    System.out.println("    v" + k.major + "." + k.minor +
    	", access=0x" + Integer.toHexString(k.access) +
	", cname=" + k.cname + ", fname=" + k.fname);

    for (int i = 0; i < k.fields.length; i++) {
	Field f = k.fields[i];
        System.out.print("  ");
	printflags(f.access);
	System.out.print(f.name + ": " + f.signature);
	if (! f.cname.equals(f.name))
	    System.out.print("  (" + f.cname + ")");
	System.out.println();
    }
    System.out.println ();
    for (int i = 0; i < k.constants.length; i++) {
	Constant c = k.constants[i];
        System.out.print(" Constant " + i + ": " + c);
	System.out.println();
    }
}



//  printmethod(m) -- dump one method.

private static void printmethod(Method m)
{
    String s;

    System.out.println();

    printflags(m.fl.access);				// modifiers and name
    System.out.println(m.fl.name + " " + m.fl.signature);

    for (int i = 0; i < m.exthrown.length; i++)
	System.out.println("throws " + m.exthrown[i].name);

    if (m.code != null) {

	for (int i = 0; i < m.handlers.length; i++) {	// handlers
	    Handler h = m.handlers[i];
	    if (h.type != null)
		s = h.type.name;
	    else
	    	s = "<all>";
	    System.out.println("  to " + h.jump + " from {" + h.start + "," +
		h.end + "} catch " + s);
	}

	printvars(m);					// variables used
    }

    // always print entry/exit stack info
    System.out.println("  args+ret     " + m.astack + " : "  + m.rstack);

    if (m.code == null)					// if no code, quit
    	return;

    for (int i = 0; i < m.instrs.length; i++) {		// for each instruction:
	System.out.print(rpad(i, 3) + ".");		//   print array index
    	printins(m.instrs[i]);				//   print instruction
	}

}



//  printvars(m) -- print local variables used in a method.

private static void printvars(Method m)
{
    int i, j;
    char c;

    System.out.print("  vars:");
    for (i = 0; i < Method.JVM_TYPES.length(); i++) {
        c = Method.JVM_TYPES.charAt(i);
	if (m.maxvar[c] >= 0) {
	    System.out.print(" " + c);
	    for (j = 0; j <= m.maxvar[c] && j < 36; j++) 
	    	if (m.varused[c].get(j))
		    System.out.print(
		    	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ".charAt(j));
	}
    }
    System.out.println();
}



//  printins(ins) -- print an instruction.

private static void printins(Instr ins)
{
    System.out.print(rpad("[" + ins.pc + "] ", 7));	// program counter [nn]

    System.out.print(ins.isReached  ? ' ' : 'X');	// 'X' if dead code
    System.out.print(ins.isTarget   ? '>' : ' ');	// '>' if jump target
    System.out.print(ins.isBoundary ? '-' : ' ');	// '-' if boundary ins
    System.out.print(" ");

    System.out.print(lpad(ins.before, 8));		// stack state (before)

    if (ins.pc == 0 || ins.label != 0)			// label, if any
    	System.out.print(lpad("L" + ins.label + ": ", 5));
    else
        System.out.print("     ");

    System.out.print(lpad(ins.opcode.name, 17));	// opcode mnemonic

    //  operand printing depends on various flags and types
    if ((ins.opcode.flags & Opcode.CTAB) != 0) {
	Object o = ins.con.value;
	if (o instanceof String)
	    System.out.print("\"" + o + "\"");
	else
	    System.out.print(o);
    } else if (0 != (ins.opcode.flags &
	    (Opcode.I8 | Opcode.I16 | Opcode.I32 | Opcode.U8 | Opcode.U16)))
    	System.out.print(ins.val);

    System.out.println();
}



//  printflags(flags) -- print access flags.

private static void printflags(int flags)
{
    PrintStream d = System.out;
    if ((flags & ClassData.ACC_PUBLIC) != 0)		d.print("public ");
    if ((flags & ClassData.ACC_PRIVATE) != 0)		d.print("private ");
    if ((flags & ClassData.ACC_PROTECTED) != 0)		d.print("protected ");
    if ((flags & ClassData.ACC_STATIC) != 0)		d.print("static ");
    if ((flags & ClassData.ACC_FINAL) != 0)		d.print("final ");
    if ((flags & ClassData.ACC_SYNCHRONIZED) != 0)    d.print("synchronized ");
    if ((flags & ClassData.ACC_VOLATILE) != 0)		d.print("volatile ");
    if ((flags & ClassData.ACC_TRANSIENT) != 0)		d.print("transient ");
    if ((flags & ClassData.ACC_NATIVE) != 0)		d.print("native ");
    if ((flags & ClassData.ACC_INTERFACE) != 0)		d.print("interface ");
    if ((flags & ClassData.ACC_ABSTRACT) != 0)		d.print("abstract ");
}



//  lpad(s, n) -- pad string on left to length of at least n (<= 20).

private static String lpad(String s, int n)
{
    if (s.length() >= n)
    	return s;
    else
    	return s + "                    ".substring(s.length(), n);
}

private static String lpad(int s, int n)  { return lpad(s + "", n); }



//  rpad(s, n) -- pad string on right to length of at least n (<= 20).

private static String rpad(String s, int n)
{
    if (s.length() >= n)
    	return s;
    else
    	return "                    ".substring(s.length(), n) + s;
}

private static String rpad(int s, int n)  { return rpad(s + "", n); }



} // class Dump
