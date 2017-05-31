//  InsGen.java -- generation of C code for bytecode instructions

package ru.dz.jpc.tanslator;

import ru.dz.jpc.classfile.*;
import java.io.*;


@Deprecated
class InsGen extends Opcode {



//  igen(d, m, ins) -- generate code for one instruction

static void igen(PrintWriter d, Method m, Instr ins)
{
    int n;
    FieldRef r;
    ClassRef c;
    String s, t1, t2;

    // We don't generate code for unreachable instructions,
    // since their stack state is indeterminate
    if (!ins.isReached)
	return;

    Opcode o = ins.opcode;

    switch (o.kind) {

	case NOP:			// nop, pop, pop2, breakpoint
	    // nothing 
	    break;

	case CONST:			// bipush, aconst_null, fconst_<n>, ...
	    d.println(assign(ins) + (ins.val + o.var) + ";");
	    break;

	case LDC:			// ldc, ldc_2, ldc2_w
	    d.println(assign(ins) + Repr.con(m, ins.con) + ";");
	    break;

	case LOAD:			// iload, aload_<n>, fload, ...
	    d.println(assign(ins) + o.name.substring(0, 1) + "v" +
		(ins.val + o.var) + ";");
	    break;

	case STORE:			// istore, fstore_<n>, astore, ...
	    s = stacktop(ins);
	    d.println("\t" + s.substring(0, 1) + "v" + (ins.val + o.var) +
		" = " + s + ";");
	    break;

	case IINC:			// iinc
	    d.println("\tiv" + ins.val + " += " + ins.more[0] + ";");
	    break;

	case GETS:			// getstatic
	case PUTS:			// putstatic
	    r = (FieldRef)ins.con.value;
	    s = Names.hashclass(r.cl.name);
	    if (!ancestor(m.cl, s))	// if not in this class or a superclass
		d.println("\tinit_" + s + "();");
	    if (o.kind == GETS)
		d.println(assign(ins) +
		    "cl_" + s + ".V." + Names.hashvar(r.name) + ";");
	    else // (o.kind == PUTS)
		d.println("\tcl_" + s + ".V." + Names.hashvar(r.name) +
		    " = " + stacktop(ins) + ";");
	    break;

	case GETF:			// getfield
	    checkref(d, ins, -1);
	    r = (FieldRef)ins.con.value;
	    d.println(assign(ins) +
		"((struct in_" + Names.hashclass(r.cl.name) + "*)" +
		stacktop(ins) + ")->" + Names.hashvar(r.name) + ";");
	    break;

	case PUTF:			// putfield
	    checkref(d, ins, -2);
	    r = (FieldRef)ins.con.value;
	    d.println("\t((struct in_" + Names.hashclass(r.cl.name) + "*)" +
		stack2nd(ins) + ")->" + Names.hashvar(r.name) + " = " +
		stacktop(ins) + ";");
	    break;

	case NEW:			// new
	    c = (ClassRef)ins.con.value;
	    d.println(assign(ins) + "new(&cl_" +
		Names.hashclass((String)c.name) + ".C);");
	    break;

	case ACAST:			// checkcas
	    c = (ClassRef)ins.con.value;
	    d.println("\tif ((" + stacktop(ins) + " != 0) && !" +
		ckinstance(ins, (String)c.name) +
		")\n\t\tthrowClassCastException(" + stacktop(ins) +");");
	    break;

	case INSTC:			// instanceof
	    c = (ClassRef)ins.con.value;
	    d.println(assign(ins) + "(" + stacktop(ins) + " != 0) && " +
		ckinstance(ins, c.name) + ";");
	    break;

	case NEWA:			// newarray
	    switch (ins.val) {
		case T_BOOLEAN:	s = "boolean";	break;
		case T_CHAR:	s = "char";	break;
		case T_FLOAT:	s = "float";	break;
		case T_DOUBLE:	s = "double";	break;
		case T_BYTE:	s = "byte";	break;
		case T_SHORT:	s = "short";	break;
		case T_INT:	s = "int";	break;
		case T_LONG:	s = "long";	break;
		default:	throw new VerifyError
				("newarray(" + ins.val + ") at pc=" + ins.pc);
	    }
	    d.println(assign(ins) +
		"anewarray(&cl_" + s + "," + stacktop(ins) + ");");
	    break;

	case ANEWA:			// anewarray
	    c = (ClassRef)ins.con.value;			// result class
	    s = c.name;
	    for (n = 0; s.charAt(n) == '['; n++)
		;					// n = class dimensions
	    if (n == 0) {
		d.println(assign(ins) + "anewarray(&cl_" +
		    Names.hashclass(s) + ".C," + stacktop(ins) + ");");
	    } else {
		d.println(assign(ins) + "vmnewarray(&" +
		    Names.classref(s.substring(n)) + "," +
		    (n + 1) + ",1," + stacktop(ins) + ");");
	    }
	    break;

	case MNEWA:			// multianewarray
	    c = (ClassRef)ins.con.value;		// result class
	    s = c.name;
	    for (n = 0; s.charAt(n) == '['; n++)
		;					// n = class dimensions
	    int nargs = ins.more[0];			// number of args
	    d.print(assign(ins) + "vmnewarray(&" +
		Names.classref(s.substring(n)) + "," + n + "," + nargs);
	    for (int i = 0; i < nargs; i++)
		d.print(",i" + (ins.after.length() + i));
	    d.println(");");
	    break;

	case ALEN:			// arraylength
	    checkref(d, ins, -1);
	    d.println(assign(ins) +
		"((struct aarray*)" + stacktop(ins) + ")->length;");
	    break;

	case ARRAYLOAD:			// iaload, faload, aaload,...
	    checkref(d, ins, -2);
	    s = o.name.substring(0,1);	// i,f,a,...
	    d.println("\tif ((unsigned)" + stacktop(ins) +
		" >= ((struct " + s + "array*)" + stack2nd(ins) +
		")->length)\n\t\tthrowArrayIndexOutOfBoundsException(" + 
		stack2nd(ins) + "," + stacktop(ins) + ");");
	    d.println(assign(ins) + "((struct " + s + "array*)" +
		stack2nd(ins) + ")->data[" + stacktop(ins) + "];");
	    break;

	case ARRAYSTORE:	       	// iastore, fastore, aastore,...
	    checkref(d, ins, -3);	
	    s = o.name.substring(0,1);	// i,f,a,...
	    d.println("\tif ((unsigned)" + stack2nd(ins) +
		" >= ((struct " + s + "array*)" + stackvar(ins,-3) +
		")->length)\n\t\tthrowArrayIndexOutOfBoundsException(" + 
		stackvar(ins,-3) + "," + stack2nd(ins) + ");");
            if ((o.flags & INST) != 0) {	// aastore must check type
                d.println("\tif (" + stacktop(ins) + " && !instanceof(" +
                    stacktop(ins) + ",((struct aarray*)" + stackAt(ins, -3) +
                    ")->class->elemclass,0))");
                d.println("\t\tthrowArrayStoreException(0);");
            }
	    d.println("\t((struct " + s + "array*)" + stackvar(ins,-3) +
		")->data[" + stack2nd(ins) + "] = " + stacktop(ins) + ";");
	    break;

	case DUP:			// dup
	    d.println(assign(ins) + stacktop(ins) + ";");
	    break;

	case DUPX1:			// dup_x1
	    // for dup_x1 to be legal, stacktop and stack2nd must be narrow vars
	    d.println("\t" + stackvar(ins.succ,-1) +" = "+ stacktop(ins) +";");
	    d.println("\t" + stackvar(ins.succ,-2) +" = "+ stack2nd(ins) +";");
	    d.println("\t"+stackvar(ins.succ,-3)+" = "+stacktop(ins.succ)+";");
	    break;

	case DUPX2:			// dup_x2
	    // stacktop must be narrow, but stack2nd may be wide
	    d.println("\t" + stackAt(ins.succ,-1) +" = "+ stackAt(ins,-1) +";");
	    d.println("\t" + stackAt(ins.succ,-2) +" = "+ stackAt(ins,-2) +";");
	    if (ins.before.charAt(ins.before.length() - 3) != 'x') { 
		// stack2nd was narrow
		d.println("\t" + stackAt(ins.succ,-3) + " = " + 
		    stackAt(ins,-3) + ";");
	    }
	   d.println("\t"+stackAt(ins.succ,-4)+" = "+ stackAt(ins.succ,-1)+";");
	    break;

	case DUP2:			// dup2
	    // stacktop can be wide or narrow; dup 2 words either way
	    d.println(assign(ins) + stacktop(ins) + ";");
	    if (ins.before.charAt(ins.before.length() - 2) != 'x') {
		// stacktop was narrow variable; duping two separate values
		d.println("\t"+stack2nd(ins.succ)+" = "+ stack2nd(ins)+";");
	    }
	    break;

	case D2X1:			// dup2x1
	    // stacktop can be wide or narrow; stackAt(ins,-3) is narrow
	    d.println(assign(ins) + stacktop(ins) + ";");
	    d.println("\t"+stackAt(ins.succ,-3)+" = "+stackAt(ins,-3)+";");
	    if (ins.before.charAt(ins.before.length() - 2) != 'x') {
		// stacktop contains a narrow variable
		d.println("\t" + stackAt(ins.succ,-2) + " = " +
		    stackAt(ins,-2) + ";");
	        d.println("\t" + stackAt(ins.succ,-5) + " = " + 
		    stackAt(ins.succ,-2) + ";");
	    }
	    d.println("\t"+stackAt(ins.succ,-4)+" = "+stackAt(ins.succ,-1)+";");
	    break;

	case D2X2:			// dup2x2
	    d.println("\t"+stackAt(ins.succ,-1)+" = "+stackAt(ins,-1)+";");
	    if (ins.before.charAt(ins.before.length() - 2) != 'x')
	       d.println("\t"+stackAt(ins.succ,-2)+" = "+stackAt(ins,-2)+";");
	    d.println("\t"+stackAt(ins.succ,-3)+" = "+stackAt(ins,-3)+";");
	    if (ins.before.charAt(ins.before.length() - 4) != 'x')
	       d.println("\t"+stackAt(ins.succ,-4)+" = "+stackAt(ins,-4)+";");
	    d.println("\t"+stackAt(ins.succ,-5)+" = "+stackAt(ins.succ,-1)+";");
	    if (ins.before.charAt(ins.before.length() - 2) != 'x')
	       d.println("\t"+stackAt(ins.succ,-6)+" = "+stackAt(ins.succ,-2)+
	       ";");
	    break;

	case SWAP:			// swap
	    n = ins.before.length();
	    t1 = ins.before.substring(n - 1);
	    t2 = ins.before.substring(n - 2, n - 1);
	    d.println("\t" + t1 + "0 = " + t1 + n + ";");
	    d.println("\t" + t2 + n + " = " + t2 + (n-1) + ";");
	    d.println("\t" + t1 + (n-1) + " = " + t1 + "0;");
	    break;

	case UNOP:			// ineg, f2d, int2short, ...
	    d.println(assign(ins) + o.opr + stacktop(ins) + ";");
	    break;

	case FTOI:			// float-to-int conversion (incl d, l)
	    d.println(assign(ins) + o.opr + "(" + stacktop(ins) + ");");
	    break;

	case DIVOP:			// {i,l}{div,rem}, but not {f,d}
	    d.println("\tif (!" + stacktop(ins) +
		") throwDivisionByZeroException();"); 
	    // no break
	case BINOP:			// iadd, fsub, dmul, ...
	    d.println(assign(ins) + stack2nd(ins) +o.opr+ stacktop(ins) + ";");
	    break;

	case FREM:			// frem, drem
	    d.println(assign(ins) +
		"remdr(" + stack2nd(ins) + "," + stacktop(ins) + ");");
	    break;

	case SHIFT:			// ishl, iushr, lshr, ...
	    s = "";				// assume no cast
	    if (o.push.length() == 2) {
		n = 0x3F;			// mask 6 bits for long shifts
		if ((o.flags & UNS) != 0)
		    s = "(ULong)";
	    } else {
		n = 0x1F;			// mask 5 bits for int shifts
		if ((o.flags & UNS) != 0)
		    s = "(UInt)";
	    }
	    d.println(assign(ins) + s + stack2nd(ins) + o.opr +
		"(" + stacktop(ins) + " & " + n + ");");
	    break;

	case CMP:			// lcmp, fcmpl, dcmpg, ...
	    // these are carefully crafted to make NaN cases come out right
	    if (o.var < 0)			// if want -1 for NaN
		d.println(assign(ins) + "(" +
		    stack2nd(ins) + " > " + stacktop(ins) + ") ? 1 : ((" +
		    stack2nd(ins) + " == " + stacktop(ins) + ") ? 0 : -1);");
	    else
		d.println(assign(ins) + "(" +
		    stack2nd(ins) + " < " + stacktop(ins) + ") ? -1 : ((" +
		    stack2nd(ins) + " == " + stacktop(ins) + ") ? 0 : 1);");
	    break;

	case IFZRO:			// ifeq, ifnull, ifgt, ...
	    d.print("\tif (" + stacktop(ins) + o.opr + "0)\n\t\t");
	    gengoto(d, m, ins, ins.val);
	    break;

	case IFCMP:			// if_icmplt, if_acmpne, ...
	    d.print("\tif (" + stack2nd(ins) +o.opr +stacktop(ins) + ")\n\t\t");
	    gengoto(d, m, ins, ins.val);
	    break;

	case TBLSW:			// tableswitch
	    n = ins.more[1] - 3;		// correction factor
	    d.println("\tswitch (" + stacktop(ins) + ") {");
	    for (int i = 3; i < ins.more.length; i++) {
		d.print("\t\tcase " + (i + n) + ": \t");
		gengoto(d, m, ins, ins.more[i]);
	    }
	    d.print("\t\tdefault:\t");
	    gengoto(d, m, ins, ins.more[0]);
	    d.println("\t}");
	    break;

	case LKPSW:			// lookupswitch
	    d.println("\tswitch (" + stacktop(ins) + ") {");
	    for (int i = 2; i < ins.more.length; i += 2) {
		d.print("\t\tcase " + ins.more[i] + ": \t");
		gengoto(d, m, ins, ins.more[i + 1]);
	    }
	    d.print("\t\tdefault:\t");
	    gengoto(d, m, ins, ins.more[0]);
	    d.println("\t}");
	    break;

	case GOTO:			// goto
	    d.print("\t");
	    gengoto(d, m, ins, ins.val);
	    break;

	case JSR:			// jsr
	    d.println("\ta" + (ins.before.length() + 1) + " = (Object)(long)" + 
		target(m, ins.pc + ins.length) + ";");
	    d.println("\tgoto L" + target(m, ins.val) + ";");
	    break;

	case RET:			// ret [to pc set by jsr]
	    // to do without macro call:
	    // d.println("\ttgt = iv" + ins.val + ";");
	    // d.println("\tgoto TOP;");
	    d.println("\tRETTO(" + ins.pc + ",(int)(long)av" + ins.val + ");");
	    break;

	case IVIRT:			// invokevirtual
	    r = (FieldRef)ins.con.value;
	    n = argindex(ins, r.signature);
	    checkref(d, ins, n + 1);
	    s = "((struct in_" +
		Names.hashclass(r.cl.name) + "*)" +
		ins.before.charAt(n) + (n + 1) + ")->class->M.\n\t\t" + 
		Names.hashmethod(r.name, r.cl.name, r.signature) + ".f";
	    invoke(d, ins, (MethodRef)r, s);
	    break;

	case INONV:			// invokenonvirtual
	    r = (FieldRef)ins.con.value;
	    checkref(d, ins, argindex(ins, r.signature) + 1);
	    invoke(d, ins, (MethodRef)r, 
		   Names.hashmethod(r.name,r.cl.name,r.signature));
	    break;

	case ISTAT:			// invokestatic
	    r = (FieldRef)ins.con.value;
	    invoke(d, ins, (MethodRef)r, 
		   Names.hashmethod(r.name,r.cl.name,r.signature));
	    break;

	case IINTR:			// invokeinterface
	    r = (FieldRef)ins.con.value;
	    n = argindex(ins, r.signature);
	    checkref(d, ins, n + 1);
	    t1 = Repr.ctype(r.signature.charAt(r.signature.indexOf(')') + 1));
	    s = "(*(" + t1 + "(*)())findinterface(a" + (n + 1) + "," +
		Names.hashinterface(r.name, r.signature) + ")->f)";
	    invoke(d, ins, (MethodRef)r, s);
	    break;

	case RETV:			// ireturn, areturn, ...
	    if (MethGen.needSwitch(m)) {
		d.println("\trv = " + stacktop(ins) + ";");
		d.println("\tgoto RETURN;");
	    } else
		d.println("\treturn " + stacktop(ins) + ";");
	    break;

	case RETRN:			// return
	    if (MethGen.needSwitch(m))
		d.println("\tgoto RETURN;");
	    else
		d.println("\treturn;");
	    break;
	
	case THROW:			// athrow
	    d.println("\tathrow(" + stacktop(ins) + ");");
	    break;

	case MENTR:			// monitorenter
	    // monitorenter(tos, thread, curpc + 1, &pc)
	    d.println("\tmonitorenter(" + stacktop(ins) + 
		", tdata, " + (ins.pc + 1) + ", &pc);");
	    break;

	case MEXIT:			// monitorexit
	    // monitorexit(tos, thread, curpc, &pc)
	    d.println("\tmonitorexit(" + stacktop(ins) + 
		", tdata, " + (ins.pc + 1) + ", &pc);");
	    break;

	default:
	    throw new VerifyError
		("unimplemented opcode " + o.name + " at pc=" + ins.pc);
    }
}



//  ancestor(cl, h) -- is the hashed classname h our class or a superclass? 

static private boolean ancestor(ClassData cl, String s)
{
    for (; cl != null; cl = cl.superclass)
	if (s.equals(cl.cname))
	    return true;
    return false;
}



//  gengoto(d, m, ins, pc) -- generate goto from ins to pc

static private void gengoto(PrintWriter d, Method m, Instr ins, int pc)
{
    int lbl = target(m, pc);

//    to do without macro calls:
//    d.println("goto L" + lbl + ";");

    if (ins.pc > pc)
	d.println("GOBACK(" + ins.pc + ",L" + lbl + ");");
    else
	d.println("GOTO(" + ins.pc + ",L" + lbl + ");");
}



//  target(m, pc) -- return label corresponding to jump target given in pc terms

static private int target(Method m, int pc)
{
    return m.instrs[m.pcmap[pc]].label;
}




//  checkref(d, ins, n) -- gen code to check that stack variable n is non-null
//
//  If n >= 0, the variable name is  "a" + n;
//  If n < 0, the variable name is   stackvar(ins,n);

static private void checkref(PrintWriter d, Instr ins, int n)
{
    String s;

    if (n >= 0)
	s = "a" + n;
    else
	s = stackvar(ins, n);

    // Make this multi-line, so we can set a breakpoint on the body
    d.println("\tif (!" + s + ") { ");
    d.println("\t\tSetNPESource(); goto NULLX;");
    d.println("\t}");
    MethGen.nullJump = true;		// set flag in method generator
}



//  ckinstance(ins, classname) -- return code that checks "instance of" relation
//
//  Let S be the type of the (non-null) object on top of the stack.
//  Let T be the target type indicated in the instruction.
//
//  The conditions under which S is an instance of T, or S is castable to T,
//  depend on type T.
//
//  If T is java.lang.Object:	always true, even if S is an array
//  If T is a class:		true if S is T or a subclass of T
//				(always true of T is java.lang.Object)
//				or if S implements T
//  If T is array of primitive:	true if S is same class as T
//				(array of same primitive type)
//  If T is array of object TC:	true if S is array of object SC and
//				SC is instance of TC

static private String ckinstance(Instr ins, String classname)
{
    // handle special case of java.lang.Object
    if (classname.equals("java.lang.Object"))
	return "1";

    // handle cast to other non-array object
    if (classname.charAt(0) != '[')
	return "(c0 = *(Class*)" + stacktop(ins) + ", c1 = &cl_" + 
	    Names.hashclass(classname) +
	    ".C,\n\t\t\t(c1->flags & 1) ? " +	// IS_INTERFACE
	    "instanceof(" + stacktop(ins) + ",c1,0) :" +
	    "\n\t\t\t\t(c0->nsupers >= c1->nsupers &&" +
	    "\n\t\t\t\tc0->supers[c0->nsupers - c1->nsupers] == c1))";

    // handle "array of primitive" case
    char c = classname.charAt(1);
    if (c != '[' && c != 'L')
	return "(*(struct class **)" + stacktop(ins) + " == &a" +
	    Names.classref(classname.substring(1)) + ".C)";

    // handle "array of object" case
    int n;
    for (n = 0; classname.charAt(n) == '['; n++)
	;					// n = class dimensions
    return "instanceof(" + stacktop(ins) +
	",&" + Names.classref(classname.substring(n)) + "," + n + ")";
}



//  invoke(d, ins, r, cname) -- generate code to invoke cname

static private void invoke(PrintWriter d, Instr ins, MethodRef r, String cname)
{
    int i = argindex(ins, r.signature);

    // start with result assignment, if any, and function name
    if (i == ins.after.length())	// if no result pushed
	d.print("\t");
    else
	d.print(assign(ins));

    // add name or expression that identifies the function
    d.print(cname + "(");

    // generate argument list
    int nargs = 0;
    for (; i < ins.before.length(); i++) {
	if (ins.before.charAt(i) != 'x') {
	    if (nargs++ != 0)
		d.print(",");
	    d.print(ins.before.substring(i, i+1) + (i + 1));
	}
    }
    d.println(");");
}



//  argindex(ins, signature) -- return index of first arg of method call

static private int argindex(Instr ins, String signature)
{
    char c = signature.charAt(signature.indexOf(')') + 1);	// return type
    int n = ins.after.length();		// stack size after call

    if (c == 'J' || c == 'D')
	return n - 2;			// long or double returns add two words
    else if (c == 'V')
	return n;			// no compensation for void return
    else
	return n - 1;			// other return types add one word
}



//  assign(ins) -- return beginning of assignment stmt ("\tvarname = ").

static final String assign(Instr ins)
{
    return "\t" + ins.after.substring(ins.after.length() - 1)
	+ ins.after.length() + " = ";
}


//  stacktop(ins) -- return name of top-of-stack variable.

static final String stacktop(Instr ins)
{
    return ins.before.substring(ins.before.length() - 1) + ins.before.length();
}


//  stack2nd(ins) -- return name of second-from-top stack variable.

static final String stack2nd(Instr ins)
{
    int len = ins.before.length();
    if (ins.before.charAt(len - 2) == 'x')	// if double-sized item on top
	return ins.before.substring(len - 3, len - 2) + (len - 2);
    else
	return ins.before.substring(len - 2, len - 1) + (len - 1);
}


//  stackvar(ins, n) -- return name of stack var at n (-1 = top, -2 = 2nd, etc.)
//
//  n counts actual stack variables, not stack positions 

static final String stackvar(Instr ins, int n)
{
    int i = ins.before.length();
    while (n < 0)
	if (ins.before.charAt(--i) != 'x')
	    n++;
    return ins.before.substring(i, i + 1) + (i + 1);
}


//  stackAt(ins, n) -- return name of stack var at position n (-1, -2, -3)
//
//  n reflects position on stack without regard to double-wide variables

static final String stackAt(Instr ins, int n)
{
    int i = ins.before.length() + n;
    return ins.before.substring(i, i + 1) + (i + 1);
}



} // class InsGen
