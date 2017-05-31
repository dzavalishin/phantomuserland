//  Method.java -- analysis of one Java method

package ru.dz.jpc.classfile;

import java.io.*;
import java.util.*;


/**
 * information about one Java method
 *
 */
public class Method {			
	
	
	
	// instance variables

	public ClassData cl;		// containing class
	public Field fl;			// method field
	public ClassRef exthrown[];		// list of exceptions thrown

	public int max_stack;		// maximum stack used
	public int max_locals;		// maximum locals used

	public byte code[];			// raw bytecode
	public Instr instrs[];		// instruction list
	
	/** pc to instruction mapping */
	public short pcmap[];		
	
	int numLabels;                     // Number of instructions that have labels
	
	/** label to instruction mapping */
	public short lblmap[];              
	
	public Handler handlers[];		// exception table
	public MethodCode mtcode;           // Information on code for this method

	public String astack;		// argument stack on entry
	public String rstack;		// stack state at time of return

	public int oflags;			// OR of flags of reachable opcodes

	//  Bit vectors record exactly which local variables are used, by JVM type.
	//  Only stores are recorded; if it's not stored, it can't be loaded.
	//  Don't forget the implicit stores of method parameters, though.

	public static BitSet[] varused;	// local variables used, by type
	public static int[] maxvar;		// highest number used, by type

	//  JVM types that appear on the stack.
	public static BitSet stktypes;	// types used on stack

	public static boolean useUnextendedBasicBlocks = false; // Change basic block def


	// manifest constants

	public static final int CHAR_INDX_SIZE = 128;  // size of array indexed by 
	// (C) chars
	public static final String JVM_TYPES="ailfd";  // Java Virtual Machine types


	/** 
	 * Assign code-generator specific information about method code.
	 * E.g., the interpreter needs only a function pointer; the JIT requires
	 * machine code, backpatch information, etc.
	 * @param mc an instance of (a subtype of) MethodCode, specific to this method
	 * @returns the parameter mc
	 */
	public MethodCode
	setMethodCode (MethodCode mc)   // Code object instance
	{
		if (null != mtcode) {
			/* Can't assign multiple code instances to one method. */
			throw new InternalError ("Multiple method code instances assigned");
		}
		mtcode = mc;
		return mc;
	}


	/**
	 *   Build method object from ClassData and field. 
	 *   For abstract or native methods, many fields are left uninitialized.
	 *   This is invoked by whoever generates code; translator/CFile, or
	 *   runtime/CodeGen on behalf of the interpreter or JIT.  CodeGen invokes
	 *   this through defineClass at load time.  The created object may need
	 *   to be revisited at link time, which is when Resolve.resolveClass is
	 *   invoked.  So we save a pointer to it in the Field parameter.
	 *   
	 * @param cl class
	 * @param fl field
	 * @throws ClassFormatError
	 */
	public Method(ClassData cl, Field fl)
	throws ClassFormatError
	{
		int c, i, n;
		Instr ins;
		Handler h;

		this.cl = cl;			// record containing class
		this.fl = fl;			// record corresponding Field struct
		/* Keep a backpointer from the field structure to the Method instance,
		 * so we can find it again */
		if (null != this.fl.tableObj) {
			throw new InternalError ("Method fields used multiply");
		}
		this.fl.tableObj = this;
		sigscan();				// always scan signature info

		// record exceptions thrown by this method (possibly none)
		byte[] xattr = Attribute.find(fl.attributes, "Exceptions");
		if (xattr == null) {
			exthrown = new ClassRef[0];
		} else try {
			ByteArrayInputStream b = new ByteArrayInputStream(xattr);
			DataInputStream d = new DataInputStream(b);
			exthrown = new ClassRef[d.readUnsignedShort()];
			for (i = 0; i < exthrown.length; i++) {
				exthrown[i] = (ClassRef)cl.constants[d.readUnsignedShort()].value;
			}
		} catch (IOException e) {
			throw new ClassFormatError(
					"Exceptions attribute of " + cl.name + "." + fl.name);
		}

		// check for abstract or native method, and bail out if no code
		if ((fl.access & (ClassData.ACC_ABSTRACT | ClassData.ACC_NATIVE)) != 0)
			return;				// nothing more to do

		// initialize
		stktypes = new BitSet();
		maxvar = new int[CHAR_INDX_SIZE];
		varused = new BitSet[CHAR_INDX_SIZE];
		for (i = 0; i < JVM_TYPES.length(); i++) {
			c = (int)JVM_TYPES.charAt(i);
			maxvar[c] = -1;
			varused[c] = new BitSet();
		}

		// mark variables that are passed as arguments
		for (i = 0; i < astack.length(); i++)  {
			c = astack.charAt(i);
			if (c == 'x') {
				markvar((char)astack.charAt(i+1), i);
				i++;
			} else
				markvar((char)c, i);
		}

		// find and unpack the "Code" attribute
		byte[] cattr = Attribute.find(fl.attributes, "Code");
		if (cattr == null)
			throw new ClassFormatError(
					"no Code attribute for " + cl.name + "." + fl.name);
		ByteArrayInputStream b = new ByteArrayInputStream(cattr);
		DataInputStream d = new DataInputStream(b);
		try {
			max_stack = d.readUnsignedShort();
			max_locals = d.readUnsignedShort();

			code = new byte[d.readInt()];
			d.readFully(code);

			// exception table
			handlers = new Handler[d.readUnsignedShort()];
			for (i = 0; i < handlers.length; i++) {
				int start = d.readUnsignedShort(),
				end = d.readUnsignedShort(),
				target = d.readUnsignedShort();
				int type = d.readUnsignedShort(); 
				ClassRef cr = null;

				if (type > 0)
					cr = (ClassRef)cl.constants[type].value;
				handlers[i] = new Handler(start, end, target, cr);
			}
		} catch (IOException e) {
			throw new ClassFormatError(
					"Code attribute of " + cl.name + "." + fl.name);
		}

		// construct instruction list
		Vector insvec = new Vector();
		pcmap = new short[code.length];

		for (int pc = 0; pc < code.length; pc += ins.length) {
			pcmap[pc] = (short)insvec.size();
			ins = new Instr(code, pc, cl.constants);
			insvec.addElement(ins);
		}
		instrs = new Instr[insvec.size()];
		insvec.copyInto(instrs);

		// set boundary bits for instructions at ends of exception ranges
		for (i = 0; i < handlers.length; i++) {
			h = handlers[i];
			instrs[pcmap[h.start]].isBoundary = true;	// mark first instr
			if (h.end < code.length)
				instrs[pcmap[h.end]].isBoundary = true;	// mark last instr
		}

		// number exception range segments, marking each instr by segment number
		if (handlers.length > 0) {
			for (i = n = 0; i < instrs.length; i++) {
				ins = instrs[i];
				if (ins.isBoundary)
					n++;
				ins.xseg = n;
			}
		}

		// mark directly reachable code, noting jump targets and stack depths
		numLabels = 0;
		mark(0, "", 0);			// begin at entry, with empty stack

		// mark exception handling code
		for (i = 0; i < handlers.length; i++)
			mark(handlers[i].jump, "a", -1);

		if (useUnextendedBasicBlocks) {
			// mark successor instructions (fall-thrus) from conditional jumps
			// as targets; we want real basic blocks, not just extended BBs.
			for (i = 1; i < instrs.length; i++) {
				ins = instrs [i-1];
				// If not already a target, and predecessor is conditional CFTI,
				// mark as target.
				if (( ! instrs [i].isTarget) &&
						((Opcode.IFZRO == ins.opcode.kind) ||
								(Opcode.IFCMP == ins.opcode.kind))) {
					++numLabels;
					instrs [i].isTarget = true;
				}
			}
		}

		// assign labels to jump targets
		n = 0;
		lblmap = new short [numLabels];
		for (i = 0; i < instrs.length; i++) {
			ins = instrs[i];
			if (ins.isTarget) {
				ins.label = n;
				lblmap [n] = (short) i;
				++n;
			}
		}
	}

	//  sigscan() -- scan method signature to set astack and rstack
	private void sigscan()
	{
		String s = fl.signature;			// signature being scanned
		StringBuffer a = new StringBuffer();	// arg stack being built
		int i = 1;					// current position (skip "(")

		if ((fl.access & ClassData.ACC_STATIC) == 0) // instance function?
			a.append('a');				// "self" parameter

		loop:
			for (i = 1; ; i++) {
				switch (s.charAt(i)) {

				case Field.FT_object:				// object
					i = s.indexOf(';', i);
					a.append('a');
					break;

				case Field.FT_array:				// array
					while (s.charAt(i) == Field.FT_array)
						i++;
					if (s.charAt(i) == Field.FT_object)
						i = s.indexOf(';', i);
					a.append('a');
					break;

					// primitive types
				case Field.FT_byte:    a.append('i');   break;
				case Field.FT_char:    a.append('i');   break;
				case Field.FT_double:  a.append("xd");  break;
				case Field.FT_float:   a.append('f');   break;
				case Field.FT_int:     a.append('i');   break;
				case Field.FT_long:    a.append("xl");  break;
				case Field.FT_short:   a.append('i');   break;
				case Field.FT_boolean: a.append('i');   break;

				case ')':  break loop;		// end of parameters
				}
			}

		astack = a.toString();

		switch (s.charAt(++i)) {
		case Field.FT_void:     rstack = "";    break;	// void
		case Field.FT_byte:     rstack = "i";   break;
		case Field.FT_char:     rstack = "i";   break;
		case Field.FT_double:   rstack = "xd";  break;
		case Field.FT_float:    rstack = "f";   break;
		case Field.FT_int:      rstack = "i";   break;
		case Field.FT_long:     rstack = "xl";  break;
		case Field.FT_short:    rstack = "i";   break;
		case Field.FT_boolean:  rstack = "i";   break;
		default:          rstack = "a";   break;
		}
	}



	//  mark(pc, stack, segnum) -- mark reachable code, maintaining stack state
	//
	//  If segnum differs from segment number of instruction at pc, the first
	//  instruction (at pc) is also marked as a boundary instruction.
	//
	//  Also marks local variables referenced by STORE-class instructions

	private void mark(int pc, String stack, int segnum)
	{
		int i = pcmap[pc];
		Instr ins = instrs[i];
		if (! ins.isTarget) {
			++numLabels;
			ins.isTarget = true;
		}
		if (ins.xseg != segnum)
			ins.isBoundary = true;

		while (true) {
			ins = instrs[i++];

			if (ins.isReached) {			// if already processed
				if (!ins.before.equals(stack))	// sanity check
					throw new VerifyError( "stack mismatch: pc=" + pc +
							" stack1=" + ins.before + " stack2=" + stack);
				return;
			}

			ins.isReached = true;			// mark instruction as processed
			oflags |= ins.opcode.flags;		// accumulate flag bits

			ins.before = stack;
			if (stack.length() > 0) {
				char c = stack.charAt(stack.length() - 1);
				stktypes.set((int)c);
				if (ins.opcode.kind == Opcode.STORE)
					markvar(c, ins.opcode.var + ins.val);
			}

			stack = ins.after = ins.nextStack();

			if ((ins.opcode.flags & Opcode.JSRI) != 0) {	// if JSR instruction
				mark(ins.val, stack + "a", ins.xseg);
				// mark JSR routine with arg
				mark(ins.pc + ins.length, stack, -1);
				// mark successor as jump tgt
			} else if ((ins.opcode.flags & Opcode.PC) != 0) // if other jump
				mark(ins.val, stack, ins.xseg); 	// mark target instrs

			if ((ins.opcode.flags & Opcode.SWCH) != 0) {	// if switch instruction
				mark(ins.more[0], stack, ins.xseg);		// mark default
				for (int j = 3; j < ins.more.length; j++) {
					mark(ins.more[j], stack, ins.xseg);	// mark other labels
					if (ins.opcode.kind == Opcode.LKPSW)
						j++;			// skip tag if lookupswitch
				}
			}

			if ((ins.opcode.flags & Opcode.NFT) != 0)
				break;				// opcode never falls through
			else
				ins.succ = instrs[i];		// note sequential successor
		}
	}



	//  markvar(jtype, n) -- mark a local variable as being used.

	private void markvar(char jtype, int n)
	{
		int i = (int)jtype;
		if (maxvar[i] < n)
			maxvar[i] = n;
		varused[i].set(n);
	}



	//  target(pc) -- return label corresponding to jump target given in pc terms

	private int target(int pc)
	{
		return instrs[pcmap[pc]].label;
	}

	public String
	toString ()
	{
		return (cl.name + "." + fl.name);
	}

} // class Method
