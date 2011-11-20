//  Instr.java -- Java bytecode instructions

package ru.dz.jpc.classfile;

import java.io.*;

public class Instr {		// one instruction in the bytecode



	// instance variables

	/** instruction label; 0 if not target */
	public int label;		
	
	public int pc;		// program counter
	public String before;	// stack state before execution of instruction
	public String after;	// stack state after execution
	public Instr succ;		// successor operation, if no branch
	public Opcode opcode;	// operation code
	public int length;		// instruction length
	public int val;		// first or only operand value
	public int[] more;		// additional values, if any
	public Constant con;	// associated constant table entry
	public boolean isTarget;	// is this instruction a jump target?
	public boolean isReached;	// is this instruction ever reached?
	public boolean isBoundary;	// is ins on range boundary (incl. by jump)?

	public int xseg;            // exception range segment number



	//  new Instr(bytecode, offset, ctab) -- build instruction instance
	//
	//  Note: relative jumps are turned into absolute PC values

	Instr(byte[] code, int offset, Constant ctab[])
	{
		int f, i, j, k, n, o, op;

		pc = offset;			// program counter
		before = after = "";		// recalculated later

		op = code[pc] & 0xFF;		// binary opcode
		opcode = Opcode.table[op];		// Opcode struct
		if (opcode == null)
			throw new VerifyError("illegal opcode " + op + " at pc=" + pc);

		if (opcode.kind == Opcode.WIDE) {	// special form: the "wide" bytecode
			length = 4;
			op = code[pc + 1] & 0xFF;
			opcode = Opcode.table[op];
			val = u2(code, pc + 2);
			if (opcode.kind == Opcode.IINC) {
				length = 6;
				more = new int[1];
				more[0] = i2(code, pc + 4);
			}
			return;
		}

		length = opcode.length;		// tableswitch, lookupswitch fixed later
		f = opcode.flags;

		if (f != 0) {

			o = offset + 1;
			if ((f & Opcode.I8) != 0)	val = i1(code, o);
			else if ((f & Opcode.I16) != 0)	val = i2(code, o);
			else if ((f & Opcode.I32) != 0)	val = i4(code, o);
			else if ((f & Opcode.U8) != 0)	val = u1(code, o);
			else if ((f & Opcode.U16) != 0)	val = u2(code, o);
			if ((f & Opcode.PC) != 0)
				val += offset;

			if ((f & Opcode.MORE) != 0) switch (opcode.kind) {
			case Opcode.IINC:
			case Opcode.MNEWA:
				more = new int[1];
				more[0] = i1(code, offset + length - 1);
				break;
			case Opcode.TBLSW:
			case Opcode.LKPSW:
				/* Set i to be the offset of the first non-opcode word in
				 * the instruction; that offset is 4-byte aligned relative
				 * to the start of the code block. */
				i = (offset + 4) & ~3;

				/* Determine the number of words (n) in the variable-length
				 * opcode, and the number of words (k) per table entry. */
				if (opcode.kind == Opcode.TBLSW) {
					/* Has a default target (i+0), table lowest value (i+4),
					 * table highest value (i+8), and (thv-tlv+1) targets. */
					n = 3 + (i4(code, i + 8) - i4(code, i + 4) + 1); 
					k = 1;
				} else {
					/* Has default target (i+0), number of pairs (i+4),
					 * and a set of n-o-p pairs of words. */
					n = 2 + 2 * i4(code, i + 4);
					k = 2;
				}
				/* Adjust length based on number of words following opcode */
				length = (i - offset) + 4 * n;
				more = new int[n];
				/* Copy the parameter words into the "more" array */
				for (j = 0; j < n; j++) {
					more[j] = i4(code, i + 4 * j);
				}
				/* Adjust the words that are addresses so they are relative
				 * to the start of the code block, not the location of the
				 * switch instruction.  (Note step skips over keys in
				 * lookupswitch instructions.) */
				more[0] += offset;
				for (j = 3; j < more.length; j+= k) {
					more[j] += offset;
				}
				break;
			case Opcode.IINTR:
				more = new int[1];
				more[0] = i1(code, offset + length - 2);
				break;
			}

			if ((f & Opcode.CTAB) != 0) {
				if (val < ctab.length)
					con = ctab[val];
				else
					throw new VerifyError(
							"bad ctab index, pc=" + offset + " val=" + val);
			}
		}

		return;
	}



	//  nextStack() -- compute stack state for successor instruction

	String nextStack()
	{
		StringBuffer s = new StringBuffer(this.before);
		char c;

		if (opcode.pop > s.length())	// sanity check
			throw new VerifyError("stack underflow: pc=" + pc +
					" stack=" + before + " opcode=" + opcode.name);

		s.setLength(s.length() - opcode.pop);
		switch (opcode.kind) {
		case Opcode.LDC:
			switch (con.tag) {
			case Constant.INTEGER:	s.append("i");	break;
			case Constant.LONG:	s.append("xl");	break;
			case Constant.FLOAT:	s.append("f");	break;
			case Constant.DOUBLE:	s.append("xd");	break;
			default:		s.append("a");	break;
			}
			break;
		case Opcode.GETF:
		case Opcode.GETS:
			c = regtype(((VariableRef)con.value).signature);
			if (c == 'l' || c == 'd')
				s.append('x');
			s.append(c);
			break;
		case Opcode.PUTF:
		case Opcode.PUTS:
			c = regtype(((VariableRef)con.value).signature);
			if (c == 'l' || c == 'd')
				s.setLength(s.length() - 1);
			break;
		case Opcode.SWAP:
			s.setLength(s.length() - 2);
			s.append(before.charAt(before.length() - 1));
			s.append(before.charAt(before.length() - 2));
			break;
		case Opcode.DUP:
			s.append(before.substring(before.length() - 1));
			break;
		case Opcode.DUP2:
			s.append(before.substring(before.length() - 2));
			break;
		case Opcode.DUPX1:
			s.insert(before.length()-2, before.substring(before.length()-1));
			break;
		case Opcode.D2X1:
			s.insert(before.length()-3, before.substring(before.length()-2));
			break;
		case Opcode.DUPX2:
			s.insert(before.length()-3, before.substring(before.length()-1));
			break;
		case Opcode.D2X2:
			s.insert(before.length()-4, before.substring(before.length()-2));
			break;
		case Opcode.MNEWA:
			s.setLength(s.length() - more[0]);
			s.append(opcode.push);
			break;
		case Opcode.IVIRT:
		case Opcode.INONV:
		case Opcode.ISTAT:
		case Opcode.IINTR:
			String sig = ((MethodRef)con.value).signature;
			String ret = sig.substring(sig.indexOf(')') + 1);
			c = regtype(ret);
			s.setLength(s.length() - argwords(sig));
			if (c == 'l' || c == 'd')
				s.append('x').append(c);
			else if (c != 'v')
				s.append(c);
			break;
		default:
			s.append(opcode.push);
		break;
		}
		return s.toString();
	}

	// argwords(instr) -- return number of argument words consumed by this call
	public int argwords ()
	{
		int na;
		MethodRef mr;

		switch (opcode.code) {
		case Opcode.IVIRT:
		case Opcode.INONV:
		case Opcode.ISTAT:
			mr = (MethodRef) con.value;
			na = Instr.argwords (mr.signature);
			break;
		case Opcode.IINTR:
			na = more[0];
			break;
		default:
			na = -1;
		}
		return na;
	}

	public static String
	argstring (String sig)
	{
		StringBuffer sb = new StringBuffer (10);

		for (int i = 1; i < sig.length(); i++) {
			switch (sig.charAt(i)) {
			case ')':	i = sig.length(); break;
			default:	throw new InternalError ("Instr.argstring: invalid char in sig" + sig);
			case 'B':	sb.append ("i");  break;
			case 'C':	sb.append ("i");  break;
			case 'D':	sb.append ("xf"); break;
			case 'F':	sb.append ("f");  break;
			case 'I':	sb.append ("i");  break;
			case 'J':	sb.append ("xl"); break;
			case 'S':	sb.append ("i");  break;
			case 'Z':	sb.append ("i");  break;
			case 'L':
				sb.append ("a");
				i = sig.indexOf (';', i);
				break;
			case '[':
				sb.append ("a");
				while (sig.charAt(++i) == '[')
					;
				if (sig.charAt(i) == 'L')
					i = sig.indexOf (';', i);
				break;
			}
		}
		return sb.toString();
	}

	//  argwords(sig) -- return number of argument words consumed by a call

	public static int argwords(String sig)
	{
		int n = 0;

		for (int i = 1; i < sig.length(); i++) {
			switch (sig.charAt(i)) {
			case ')':	return n;
			default:	n += 1;  break;	// error
			case 'B':	n += 1;  break;
			case 'C':	n += 1;  break;
			case 'D':	n += 2;  break;
			case 'F':	n += 1;  break;
			case 'I':	n += 1;  break;
			case 'J':	n += 2;  break;
			case 'S':	n += 1;  break;
			case 'Z':	n += 1;  break;
			case 'L':
				n += 1;
				while (sig.charAt(++i) != ';')
					;	
				break;
			case '[':
				n += 1;
				while (sig.charAt(++i) == '[')
					;
				if (sig.charAt(i) == 'L')
					while (sig.charAt(++i) != ';')
						;	
				break;
			}
		}
		return n;
	}



	//  regtype(sig) -- return register type character for field signature

	static char regtype(String sig)
	{
		switch (sig.charAt(0)) {
		case 'B':  return 'i';
		case 'C':  return 'i';
		case 'D':  return 'd';
		case 'F':  return 'f';
		case 'I':  return 'i';
		case 'J':  return 'l';
		case 'S':  return 'i';
		case 'V':  return 'v';
		case 'Z':  return 'i';
		default:   return 'a';
		}
	}



	//  u1(code, offset) -- load 1-byte unsigned integer from code.
	//  u2(code, offset) -- load 2-byte unsigned integer from code.
	//  i1(code, offset) -- load 1-byte signed integer from code.
	//  i2(code, offset) -- load 2-byte signed integer from code.
	//  i4(code, offset) -- load 4-byte signed integer from code.

	static int u1(byte code[], int offset)
	{
		return code[offset] & 0xFF;
	}

	static int u2(byte code[], int offset)
	{
		return ((code[offset] & 0xFF) << 8) | (code[offset + 1] & 0xFF);
	}

	static int i1(byte code[], int offset)
	{
		return code[offset];
	}

	static int i2(byte code[], int offset)
	{
		return (code[offset] << 8) | (code[offset + 1] & 0xFF);
	}

	static int i4(byte code[], int offset)
	{
		return (code[offset] << 24) | ((code[offset + 1] & 0xFF) << 16) |
		((code[offset + 2] & 0xFF) << 8) | (code[offset + 3] & 0xFF);
	}


	public final synchronized String
	toString ()
	{
		StringBuffer sb;

		/* 2c: Mark boundaries with > */
		sb = new StringBuffer (isBoundary ? "> " : "  ");
		/* 6c: Label, if present */
		if (isTarget) {
			if (10 > label) {
				sb.append ("L00" + label + ": ");
			} else if (100 > label) {
				sb.append ("L0"  + label + ": ");
			} else {
				sb.append ("L"   + label + ": ");
			}
		} else {
			sb.append (isReached ? "      " : " xxxx ");
		}

		/* 3c: program counter */
		if (10 > pc) {
			sb.append ("  " + pc);
		} else if (100 > pc) {
			sb.append (" " + pc);
		} else {
			sb.append ("" + pc);
		}
		/* 14c: before stack */
		sb.append (".  ").append (before).append ("           ".substring (before.length()));
		/* 20c: opcode name */
		sb.append (opcode.name).append ("                    ".substring (opcode.name.length()));
		/* Rem: instruction val */
		if (0 != (opcode.flags & ~Opcode.NFT)) {
			sb.append (val);
		}
		return sb.toString ();
	}

	//  dump(d, list) -- dump list of instructions on a file as C comments

	public static void dump(PrintWriter d, Instr list[])
	{
		d.println();
		d.println("/*");
		for (int i = 0; i < list.length; i++) {
			Instr ins = list[i];
			d.println (ins.toString());
		}
		d.println("*/");
		d.println();
	}



} // class Instr
