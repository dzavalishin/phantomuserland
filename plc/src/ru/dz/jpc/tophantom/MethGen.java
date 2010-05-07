//  MethGen.java -- generation of C code for Java methods

package ru.dz.jpc.tophantom;

import ru.dz.jpc.classfile.*;
import ru.dz.plc.compiler.ClassMap;
import ru.dz.plc.compiler.ClassTable;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

import java.io.*;
import java.util.*;



class MethGen extends Opcode {



	static boolean nullJump;	// flag, set by some InsGens



	//  needSwitch(m) -- does method m generate a switch() statement?

	static final boolean needSwitch(Method m) {
		return m.handlers.length > 0 || (m.oflags & JSRI) != 0;
	}



	//  minfo(d, method) -- generate info, even for native method.

	static void minfo(PrintWriter d, Method m, PhantomClass pclass )
	{
		Field f = m.fl;

		// generate comment giving hashed and unhashed names
		d.println();
		d.println("/*M " + f.cname + ": " +
				m.cl.name + "." + f.name + f.signature + " */");

		// generate list of thrown exceptions
		d.println();
		/* We'd like this thing to be static, since nobody else should care
		 * about it, but the Irix C compiler won't let us get away with that
		 * because we had to refer to the thing earlier in the file and it
		 * made us declare it extern beforehand.  See CFile.java. */
		d.print("Class xt_" + f.cname + "[] = { ");
		for (int i = 0; i < m.exthrown.length; i++) {
			if (i % 3 == 2) 
				d.print("\n    ");
			d.print("&cl_" + Names.hashclass(m.exthrown[i].name) + ".C, ");
		}
		d.println("0 };");
	}



	//  mgen(d, method) -- generate code for one method.

	static void mgen(PrintWriter d, Method m, PhantomClass pclass, ParseState ps ) throws PlcException
	{
		ClassData cls = m.cl;
		Field f = m.fl;
		int n;

		//NodeStack nodeStack = new NodeStack(); // Used to conver Java stack ops to Phantom tree

		NodeEmitter emitter = new NodeEmitter();
		
		ru.dz.plc.compiler.Method pMethod = pclass.addMethod( f.name, 
				new PhantomType( ClassMap.get_map().get(".internal.object",false,null) ) 
		);

		ps.set_method(pMethod);


		// dump the code as C comments, if requested
		if (Trans.debugging(Trans.dbgCode))
			Instr.dump(d, m.instrs);

		// generate the function header
		d.println();
		d.print(Repr.rettype(m.fl.signature) + " ");
		if ((f.access & ClassData.ACC_SYNCHRONIZED) != 0)
			d.print("sy_");
		d.println(f.cname + "(" + parmdecls(m) + ")");
		d.println("{");

		// generate the exception table, if any
		if (m.handlers.length > 0) {
			d.println("static struct handler htable[] = {");
			for (int i = 0; i < m.handlers.length; i++) {
				Handler h = m.handlers[i];
				if (h.type == null)
					d.print("    0, ");
				else
					d.print("    &cl_" +
							Names.hashclass(h.type.name) +
					".C, ");
				d.println(h.start + ", " + h.end + ", " +
						m.instrs[m.pcmap[h.jump]].label + ",");
			}
			d.println("};");
		}

		// declare variables for exception handling and JSR
		if (m.handlers.length > 0) {
			d.println("struct mythread *tdata;");
			d.println("jmp_buf newbuf;");
			d.println("void *oldbuf;");
			d.println("volatile int pc;");
		}
		if (needSwitch(m))
			d.println("int tgt;");
		if (needSwitch(m) && m.rstack.length() > 0)
			d.println(Repr.rettype(m.fl.signature) + " rv;");

		// declare stack and local variables
		if ((m.oflags & Opcode.INST) != 0)		// need temps for ckinstance()?
			d.println("Class c0, c1;");
		for (int i = 0; i < Method.JVM_TYPES.length(); i++) {
			char c = Method.JVM_TYPES.charAt(i);
			dclstack(d, m, c);
			dclvars(d, m, c, m.handlers.length > 0);
		}

		/* Insert macro for generic function prologue. */
		d.println("PROLOGUE;");

		// generate initialization of local variables (from parameter list)
		d.println();
		lvinits(d, m, pclass, pMethod);				

		// initialize class, if static function, and if class has initializer
		if ((f.access & ClassData.ACC_STATIC) != 0
				&& !f.name.equals("<clinit>")
				&& cls.getmethod("<clinit>",false) != null) {
			d.println();
			d.println("\tif (cl_" + cls.cname + ".C.needinit)");
			d.println("\t\tinitclass(&cl_" + cls.cname + ".C);");
		}

		// initialize exception handling
		if (m.handlers.length > 0) {
			d.println();
			d.println("\ttdata = mythread();");
			d.println("\toldbuf = tdata->jmpbuf;");
			d.println("\ttgt = 0;");
			d.println("\tif (setjmp(newbuf)) {");
			d.println("\t\tsthread_got_exception();");
			d.println("\t\ta1 = tdata->exception;");
			d.println("\t\tif ((tgt = findhandler(htable, " + m.handlers.length +
			", a1, pc)) < 0)");
			d.println("\t\t\tlongjmp(oldbuf, 1);");
			d.println("\t}");
			d.println("\ttdata->jmpbuf = newbuf;");
		} else if (needSwitch(m)) {
			d.println();
			d.println("\ttgt = 0;");
		}

		// generate switch top
		if (needSwitch(m)) {
			d.println();
			d.println("TOP: switch(tgt) {");
		}

		// generate code for individual instructions
		d.println();
		nullJump = false;			// assume we won't need a null jump

		for (int i = 0; i < m.instrs.length; i++) {
			Instr ins = m.instrs[i];
			if (ins.isTarget) {
				d.print("L" + ins.label + ":");	// generate label
				if (needSwitch(m))			// and also case label if needed
					d.println("  case " + ins.label + ":");
			}
			if (ins.isBoundary && m.handlers.length > 0)
				d.println("\tpc = " + ins.pc + ";");
			if (Trans.debugging(Trans.dbgInstrs))
				d.println("\t\t\t/* " + ins.opcode.name + " */");
			InsGen.igen(d, emitter, m, ins, pclass, ps);			// generate code for instruction
			if ((ins.opcode.flags & Opcode.NFT) != 0)
				d.println();		// if no fallthrough, add visual break
		}

		// terminate switch
		if (needSwitch(m)) {
			d.println("}");
			d.println("RETURN:");
			d.println("\ttdata->jmpbuf = oldbuf;");
			if (m.rstack.length() > 0)
				d.println("\treturn rv;");
			else
				d.println("\treturn;");
		}

		// null pointer exceptions need a jump target
		if (nullJump) {
			d.println("NULLX:");
			d.println("\tthrowNullPointerException(0);");
		}

		// To shut up the Irix C compiler, let's make it clear that we don't
		// actually ever get here. */
		d.println("\t/*NOTREACHED*/");
		d.println("}");



		Node node = emitter.getCodeTree();   

		node.preprocess(ps);

		pMethod.code = node;

		ps.set_method(null);

	}



	//  parmdecls(m) -- return string of C parameter declarations for Java method m.

	private static String parmdecls(Method m)
	{
		String s = m.fl.signature;			// signature being scanned
		int i = 1;					// current position (skip "(")
		String t;

		StringBuffer b = new StringBuffer();	// parm list being built
		int lv = 0;					// local variable count

		if ((m.fl.access & ClassData.ACC_STATIC) == 0) {  // instance function?
			b.append("Object p0, ");		// "self" parameter
		}

		loop:
			for (int n = 1; ; n++) {
				char c = s.charAt(i);
				switch (c) {

				case 'L':				// object
				default:				// not used, keeps javac happy
					t = "Object";
				i = s.indexOf(';', i);
				break;

				case '[':				// array
					t = "Object";
					while (s.charAt(i) == '[')
						i++;
					if (s.charAt(i) == 'L')
						i = s.indexOf(';', i);
					break;

					// primitive types
				case 'B':  t = "Byte";     break;
				case 'C':  t = "Char";     break;
				case 'D':  t = "Double";   break;
				case 'F':  t = "Float";    break;
				case 'I':  t = "Int";      break;
				case 'J':  t = "Long";     break;
				case 'S':  t = "Short";    break;
				case 'Z':  t = "Boolean";  break;

				case ')':  break loop;		// end of parameters
				}
				b.append(t).append(" p").append(n).append(", ");
				i++;
			}

		if (b.length() == 0)
			return "void";				// no parameters
		else {
			b.setLength(b.length() - 2);		// trim final ", " from string
			return b.toString();			// return param declarations
		}
	}



	//  dclstack(d, m, jtype) -- declare stack variables for a JVM type

	static private void dclstack(PrintWriter d, Method m, char jtype)
	{
		if (!m.stktypes.get((int)jtype))
			return;

		d.print(Repr.ctype(jtype) + " ");
		for (int i = 0; i < m.max_stack; i++) {
			if (i > 0 && i % Repr.DECLS_PER_LINE == 0)
				d.print("\n    ");
			d.print(jtype + Integer.toString(i) + ", ");
		}
		d.println(jtype + Integer.toString(m.max_stack) + ";");
	}



	//  dclvars(d, m, jtype, volatile) -- declare local variables marked in bitset

	static private void dclvars(PrintWriter d, Method m, char jtype, boolean vol)
	{
		int max = m.maxvar[(int)jtype];
		if (max < 0)
			return;

		BitSet bs = m.varused[(int)jtype];
		String s;
		int n = 0;

		for (int i = 0; i <= max; i++) {
			if (bs.get(i)) {
				s = " " + jtype + "v" + i;
				if (n % Repr.DECLS_PER_LINE == 0) {
					if (n == 0) {
						if (vol)
							d.print("volatile ");
						d.print(Repr.ctype(jtype) + s);
					} else
						d.print(",\n   " + s);
				} else
					d.print("," + s);
				n++;
			}
		}
		d.println(";");
	}



	//  lvinits(d, m) -- generate local variable initializations for method m

	static private void lvinits(PrintWriter d, Method m, PhantomClass pc, ru.dz.plc.compiler.Method pm ) throws PlcException
	{
		String s = m.astack;				// entry "stack"
		int pnum = 0;					// parameter number
		int vnum = 0;					// variable number

		if ((m.fl.access & ClassData.ACC_STATIC) != 0)	// if static function
			pnum++;						//    no "this" param

		//int phantomOrdinal = 0;

		boolean isInt = false;

		for (int i = 0; i < s.length(); i++) {		// for each entry val
			String varName;

			char c = s.charAt(i);
			if (c == 'x') {					// if double-wide 
				c = s.charAt(++i);
				d.println("\t" + c + "v" + vnum + " = p" + pnum++ + ";");

				varName = c + "v" + vnum;

				vnum += 2;
			} else {
				varName = c + "v" + vnum;
				d.println("\t" + c + "v" + vnum++ + " = p" + pnum++ + ";");
			}
			/*
			pc.setField( phantomOrdinal, varName, null);
			System.out.println("set parm field "+varName);
			 */

			if(i == 0 )
			{
				// First one is 'this', skip it
				continue;
			}
			
			isInt = c == 'i';				

			PhantomClass vc = isInt ?
					ClassMap.get_map().get(".internal.int",false,null) :
						ClassMap.get_map().get(".internal.object",false,null);

					pm.addArg(varName, new PhantomType(vc) );
		}

	}


	//  syncwrap(d, m) -- generate wrapper function for synchronized method

	static void syncwrap(PrintWriter d, Method m)
	{
		int i, j;
		String p;

		// generate header
		p = parmdecls(m);
		d.println("\n\n");
		d.println("/* wrapper for synchronized method */");
		d.println(Repr.rettype(m.fl.signature) + " " + m.fl.cname + "(" + p + ")");
		d.println("{");

		// declare underlying function, if native
		if ((m.fl.access & ClassData.ACC_NATIVE) != 0)
			d.println("\t" + Repr.rettype(m.fl.signature) +
					" sy_" + m.fl.cname + "();");

		// declare return value, if not void method
		if (m.rstack.length() > 0)
			d.println("\t" + Repr.rettype(m.fl.signature) + " rv;");

		// declare data pointer, sigsetjmp buffer, and variable
		d.println("\tstruct mythread *tdata;");
		d.println("\tjmp_buf newbuf;");
		d.println("\tvoid *oldbuf;");
		d.println("\tvolatile int monitor_held = 0;");
		d.println("\t");

		// set exception catcher
		d.println("\ttdata = mythread();");
		d.println("\toldbuf = tdata->jmpbuf;");
		d.println("\tif (setjmp(newbuf)) {");
		d.println("\t\tsthread_got_exception();");

		// on exception: exit monitor, if held, and rethrow exception
		d.println("\t\tif (monitor_held)");
		if ((m.fl.access & ClassData.ACC_STATIC) != 0) {
			d.println("\t\t\texitclass(&cl_" +
					m.cl.cname + ".C, tdata, 0, &monitor_held);");
		} else {
			d.println("\t\t\tmonitorexit(p0, tdata, 0, &monitor_held);");
		}
		d.println("\t\ttdata->jmpbuf = oldbuf;");
		d.println("\t\tathrow(tdata->exception);");
		d.println("\t}");

		// point exception handling to this jump buffer
		d.println("\ttdata->jmpbuf = newbuf;");

		// enter the monitor now that the exception handler is prepared
		if ((m.fl.access & ClassData.ACC_STATIC) != 0) {
			// for static functions, the class is locked
			d.println("\tenterclass(&cl_" + 
					m.cl.cname + ".C, tdata, 1, &monitor_held);");
		} else {
			// for instance functions, the instance (p0) is locked.
			d.println("\tmonitorenter(p0, tdata, 1, &monitor_held);");
		}

		// call underlying function
		if (m.rstack.length() > 0)
			d.print("\trv = ");		// assign return value, if any
		else
			d.print("\t");
		d.print("sy_" + m.fl.cname + "(");	// underlying function name
		i = -1;
		while ((i = p.indexOf('p', i + 1)) >= 0) {
			j = p.indexOf(',', i + 1) + 1;	// parameters are pnnn through comma
			if (j <= 0)
				j = p.length();
			d.print(p.substring(i, j));
		}
		d.println(");");

		// exit monitor
		if ((m.fl.access & ClassData.ACC_STATIC) != 0) {
			d.println("\texitclass(&cl_" 
					+ m.cl.cname + ".C, tdata, 0, &monitor_held);");
		} else {
			d.println("\tmonitorexit(p0, tdata, 0, &monitor_held);");
		}

		// restore old exception handler and return
		d.println("\ttdata->jmpbuf= oldbuf;");
		if (m.rstack.length() > 0)
			d.println("\treturn rv;");
		d.println("}");
	}



} // class MethGen
