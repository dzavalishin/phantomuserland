//  InsGen.java -- generation of C code for bytecode instructions

package ru.dz.jpc.tophantom;

import java.io.PrintWriter;

import ru.dz.jpc.classfile.ClassData;
import ru.dz.jpc.classfile.ClassRef;
import ru.dz.jpc.classfile.Constant;
import ru.dz.jpc.classfile.FieldRef;
import ru.dz.jpc.classfile.Instr;
import ru.dz.jpc.classfile.Method;
import ru.dz.jpc.classfile.MethodRef;
import ru.dz.jpc.classfile.Names;
import ru.dz.jpc.classfile.Opcode;
import ru.dz.jpc.tophantom.node.IdentTransNode;
import ru.dz.jpc.tophantom.node.binode.OpAssignTransNode;
import ru.dz.jpc.tophantom.node.binode.OpPlusTransNode;
import ru.dz.jpc.tophantom.node.binode.ValEqTransNode;
import ru.dz.jpc.tophantom.node.binode.ValGeTransNode;
import ru.dz.jpc.tophantom.node.binode.ValGtTransNode;
import ru.dz.jpc.tophantom.node.binode.ValLeTransNode;
import ru.dz.jpc.tophantom.node.binode.ValLtTransNode;
import ru.dz.jpc.tophantom.node.binode.ValNeqTransNode;
import ru.dz.plc.compiler.ClassMap;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeInt;
import ru.dz.plc.compiler.PhTypeObject;
import ru.dz.plc.compiler.PhTypeString;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomStackVar;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.PhantomVariable;
import ru.dz.plc.compiler.binode.CallArgNode;
import ru.dz.plc.compiler.binode.NewNode;
import ru.dz.plc.compiler.binode.OpAndNode;
import ru.dz.plc.compiler.binode.OpAssignNode;
import ru.dz.plc.compiler.binode.OpDivideNode;
import ru.dz.plc.compiler.binode.OpMinusNode;
import ru.dz.plc.compiler.binode.OpMultiplyNode;
import ru.dz.plc.compiler.binode.OpOrNode;
import ru.dz.plc.compiler.binode.OpSubscriptNode;
import ru.dz.plc.compiler.binode.SequenceNode;
import ru.dz.plc.compiler.node.DupDestNode;
import ru.dz.plc.compiler.node.DupSourceNode;
import ru.dz.plc.compiler.node.EmptyNode;
import ru.dz.plc.compiler.node.IntConstNode;
import ru.dz.plc.compiler.node.JumpNode;
import ru.dz.plc.compiler.node.JumpTargetNode;
import ru.dz.plc.compiler.node.JzNode;
import ru.dz.plc.compiler.node.MethodNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.NullNode;
import ru.dz.plc.compiler.node.ReturnNode;
import ru.dz.plc.compiler.node.StringConstNode;
import ru.dz.plc.compiler.node.ThisNode;
import ru.dz.plc.compiler.node.ThrowNode;
import ru.dz.plc.compiler.node.VoidNode;
import ru.dz.plc.compiler.trinode.OpMethodCallNode;
import ru.dz.plc.parser.ParserContext;
import ru.dz.plc.util.PlcException;


class InsGen extends Opcode {


	
	

	//  igen(d, m, ins) -- generate code for one instruction

	static void igen(PrintWriter d, NodeEmitter ns, Method m, Instr ins, PhantomClass myClass, ParseState ps) throws PlcException
	{
		int n;
		FieldRef r;
		ClassRef c;
		String s, t1, t2;

		// We don't generate code for unreachable instructions,
		// since their stack state is indeterminate
		if (!ins.isReached)
			return;

		ParserContext pcontext = new ParserContext("", ins.pc);

		Opcode o = ins.opcode;

		if(ins.label != 0)
		{
            ns.push( new JumpTargetNode(ins.label) );
		}
		
		String ident = o.name.substring(0, 1) + "v" + (ins.val + o.var);
		switch (o.kind) {

		case NOP:			// nop, pop, pop2, breakpoint
			// nothing
			if( o.pop > 0 )
			{
				throw new PlcException("have to pop here");
			}

			// TODO check that pop/pop2 are processed

			break;

		case CONST:			// bipush, aconst_null, fconst_<n>, ...
			d.println(assign(ins) + (ins.val + o.var) + ";");

			{
                ns.push(new IntConstNode( ins.val + o.var ));
//				ns.push(new IntConstNode( ins.val ));
				return;

				/*
				Constant ct = m.cl.constants[o.var];

				switch(ct.tag)
				{
				case Constant.INTEGER:
					ns.push(new IntConstNode( (Integer)ct.value ));
					return;

				case Constant.STRING:
					ns.push(new StringConstNode( (String)ct.value ));
					return;
				}
				 */

			}
			//break;

		case LDC:			// ldc, ldc_2, ldc2_w
			d.println(assign(ins) + Repr.con(m, ins.con) + ";");
            {
                switch (ins.con.tag) {
                    case Constant.STRING :
                        ns.push(new StringConstNode( (String)ins.con.value ));
                        return;
                    case Constant.INTEGER :
                        break;                       // not supported yet

                    case Constant.CLASS :
                    case Constant.FIELD :
                    case Constant.METHOD :
                    case Constant.INTERFACE :
                    case Constant.FLOAT :
                    case Constant.LONG :
                    case Constant.DOUBLE :
                    case Constant.NAMETYPE :
                    case Constant.UTF8 :
                    case Constant.UNICODE :
                    default:
                        break;                       // not supported yet
                }
            }
			break;

		case LOAD:			// iload, aload_<n>, fload, ...
			d.println(assign(ins) + ident + ";");
			{
                ensureAutoVariable(ps, ident);

				if(o.name.equals("aload_0"))
					ns.push(new ThisNode(myClass));
				else
					ns.push(new IdentTransNode(ident));
			}
			
			return;

		case STORE:			// istore, fstore_<n>, astore, ...
			s = stacktop(ins);
			d.println("\t" + s.substring(0, 1) + "v" + (ins.val + o.var) +
					" = " + s + ";");

            ensureAutoVariable(ps, ident);

			ns.push( new OpAssignTransNode( new IdentTransNode(ident), ns.pop()) );

			return;

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

		case GETF:			// [getfield]
			checkref(d, ins, -1);
			r = (FieldRef)ins.con.value;
			d.println(assign(ins) +
					"((struct in_" + Names.hashclass(r.cl.name) + "*)" +
					stacktop(ins) + ")->" + Names.hashvar(r.name) + ";");

            ns.push(new IdentTransNode(r.name));
//            {
//                myClass.addField(r.name, getFieldType(r.signature));
//            }
			return;

		case PUTF:			// putfield
			checkref(d, ins, -2);
			r = (FieldRef)ins.con.value;
			d.println("\t((struct in_" + Names.hashclass(r.cl.name) + "*)" +
					stack2nd(ins) + ")->" + Names.hashvar(r.name) + " = " +
					stacktop(ins) + ";");

//			ns.push( new OpAssignNode( new IdentTransNode(r.name), ns.pop()) );
            ns.push( new OpAssignTransNode( new IdentTransNode(r.name), ns.pop()) );

			return;

		case NEW:			// new
			c = (ClassRef)ins.con.value;
			d.println(assign(ins) + "new(&cl_" +
					Names.hashclass((String)c.name) + ".C);");
			{
				PhantomClass pc = ClassMap.get_map().get(c.name,false,null);
				if (pc == null)
					throw new PlcException("Class " + c.name + " is not defined - forgot to import?");

				PhantomType type = new PhantomType(pc);

				ns.push(new NewNode(type,null,null));

			}
			return;

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

		case NEWA:			// [newarray]
// toba [
//			switch (ins.val) {
//			case T_BOOLEAN:	s = "boolean";	break;
//			case T_CHAR:	s = "char";	break;
//			case T_FLOAT:	s = "float";	break;
//			case T_DOUBLE:	s = "double";	break;
//			case T_BYTE:	s = "byte";	break;
//			case T_SHORT:	s = "short";	break;
//			case T_INT:	s = "int";	break;
//			case T_LONG:	s = "long";	break;
//			default:	throw new VerifyError
//			("newarray(" + ins.val + ") at pc=" + ins.pc);
//			}
//			d.println(assign(ins) +
//					"anewarray(&cl_" + s + "," + stacktop(ins) + ");");
// toba ]
            {
//                switch (ins.val) {
//                case T_BOOLEAN:	s = "boolean";	break;
//                case T_CHAR:	s = "char";	break;
//                case T_FLOAT:	s = "float";	break;
//                case T_DOUBLE:	s = "double";	break;
//                case T_BYTE:	s = "byte";	break;
//                case T_SHORT:	s = "short";	break;
//                case T_INT:	s = "int";	break;
//                case T_LONG:	s = "long";	break;
//                default:	throw new VerifyError("newarray(" + ins.val + ") at pc=" + ins.pc);
//                }
//                PhantomType type = getPhantomType(s);
                PhantomType arrayType = getVoidArrayType();

                ensureAutoVariable(ps, ident);

                ns.pop(); // remove array size
                NewNode newNode = new NewNode(arrayType, null, null);
                ns.push(newNode);
            }
            return;

		case ANEWA:			// [anewarray]
// toba [
//			c = (ClassRef)ins.con.value;			// result class
//			s = c.name;
//			for (n = 0; s.charAt(n) == '['; n++)
//				;					// n = class dimensions
//			if (n == 0) {
//				d.println(assign(ins) + "anewarray(&cl_" +
//						Names.hashclass(s) + ".C," + stacktop(ins) + ");");
//			} else {
//				d.println(assign(ins) + "vmnewarray(&" +
//						Names.classref(s.substring(n)) + "," +
//						(n + 1) + ",1," + stacktop(ins) + ");");
//			}
// toba ]

            {
//                PhantomType type = getPhantomType(c.name);
                PhantomType arrayType = getVoidArrayType();

                ensureAutoVariable(ps, ident);

                ns.pop(); // remove array size
                NewNode newNode = new NewNode(arrayType, null, null);
                ns.push(newNode);
            }
            return;
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
// toba [
//			checkref(d, ins, -2);
//			s = o.name.substring(0,1);	// i,f,a,...
//			d.println("\tif ((unsigned)" + stacktop(ins) +
//					" >= ((struct " + s + "array*)" + stack2nd(ins) +
//					")->length)\n\t\tthrowArrayIndexOutOfBoundsException(" +
//					stack2nd(ins) + "," + stacktop(ins) + ");");
//			d.println(assign(ins) + "((struct " + s + "array*)" +
//					stack2nd(ins) + ")->data[" + stacktop(ins) + "];");
// toba ]

            {
//                ensureAutoVariable(ps, ident);
                Node arrayIndex = ns.pop();
                IdentTransNode identVar = (IdentTransNode)ns.pop();

//                PhantomType arrayType = getArrayType(o.name);
                PhantomType arrayType = getVoidArrayType();

                identVar.setType(arrayType);
                updateArrayTypeStackVariable(ps, identVar.get_name(), arrayType);
                
                OpSubscriptNode array = new OpSubscriptNode(identVar, arrayIndex);
                ns.push(array);
            }
            return;

		case ARRAYSTORE:	       	// iastore, fastore, aastore,...
// toba [
//			checkref(d, ins, -3);
//			s = o.name.substring(0,1);	// i,f,a,...
//			d.println("\tif ((unsigned)" + stack2nd(ins) +
//					" >= ((struct " + s + "array*)" + stackvar(ins,-3) +
//					")->length)\n\t\tthrowArrayIndexOutOfBoundsException(" +
//					stackvar(ins,-3) + "," + stack2nd(ins) + ");");
//			if ((o.flags & INST) != 0) {	// aastore must check type
//				d.println("\tif (" + stacktop(ins) + " && !instanceof(" +
//						stacktop(ins) + ",((struct aarray*)" + stackAt(ins, -3) +
//				")->class->elemclass,0))");
//				d.println("\t\tthrowArrayStoreException(0);");
//			}
//			d.println("\t((struct " + s + "array*)" + stackvar(ins,-3) +
//					")->data[" + stack2nd(ins) + "] = " + stacktop(ins) + ";");
// toba ]

            {
//                ensureAutoVariable(ps, ident);
                Node value = ns.pop();
                Node arrayIndex = ns.pop();
                IdentTransNode identVar = (IdentTransNode)ns.pop();

//                PhantomType arrayType = getArrayType(o.name);
                PhantomType arrayType = getVoidArrayType();

                identVar.setType(arrayType);
                updateArrayTypeStackVariable(ps, identVar.get_name(), arrayType);

                OpSubscriptNode array = new OpSubscriptNode(identVar, arrayIndex);
                OpAssignNode assign = new OpAssignTransNode(array, value);
                ns.push(assign);
            }
            return;
		case DUP:			// dup
			d.println(assign(ins) + stacktop(ins) + ";");

			Node expr = ns.pop();
			ns.push(new DupDestNode(expr)); // 2nd pop will generate no code and use orig 
			ns.push(new DupSourceNode(expr)); // First pop will generate dup instr and use a copy

			return;

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

			{
				Node stackTopNode = ns.pop();
				Node op = null;

				if(o.opr.equals("ineg")) op = new OpMinusNode(new IntConstNode(0),stackTopNode);
				else
					break;

				ns.push(op);
			}

			break;

		case FTOI:			// float-to-int conversion (incl d, l)
			d.println(assign(ins) + o.opr + "(" + stacktop(ins) + ");");
			break;

		case DIVOP:			// {i,l}{div,rem}, but not {f,d}
			d.println("\tif (!" + stacktop(ins) +
			") throwDivisionByZeroException();"); 
			// no break
		case BINOP:			// [iadd, isub, imul, idiv]
			d.println(assign(ins) + stack2nd(ins) +o.opr+ stacktop(ins) + ";");

			{
                Node operand2 = ns.pop();
                Node operand1 = ns.pop();

				Node op = null;

				if(o.name.equals("iadd")) op = new OpPlusTransNode(operand1,operand2);
//				if(o.name.equals("iadd")) op = new OpPlusNode(stackTopNode,stack2ndNode);
                else if(o.name.equals("isub")) op = new OpMinusNode(operand1,operand2);
                else if(o.name.equals("imul")) op = new OpMultiplyNode(operand1,operand2);
                else if(o.name.equals("idiv")) op = new OpDivideNode(operand1,operand2);
                else if(o.name.equals("iand")) op = new OpAndNode(operand1,operand2);
                else if(o.name.equals("ior")) op = new OpOrNode(operand1,operand2);
				else
				{
					System.out.println("binop: "+o.name);
					break;
				}

				ns.push(op);
			}			
			return;

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

		case IFZRO:			// [ifeq, ifne, ifle, iflt, ifge, ifgt, ifnonnull, ifnull]
			d.print("\tif (" + stacktop(ins) + o.opr + "0)\n\t\t");
			gengoto(d, m, ins, ins.val);

            {
                Node p1 = ns.pop();
                Node p2 = (o.name.equals("ifnonnull") || o.name.equals("ifnull")) ? new NullNode() : new IntConstNode(0);

				String cmpop = o.opr.trim();

				// Reverse op! We use JZ, not JNZ!
				if(cmpop.equals("<="))      ns.push(new ValGtTransNode(p1,p2));
				else if(cmpop.equals("<"))  ns.push(new ValGeTransNode(p1,p2));
				else if(cmpop.equals(">=")) ns.push(new ValLtTransNode(p1,p2));
				else if(cmpop.equals(">"))  ns.push(new ValLeTransNode(p1,p2));
                else if(cmpop.equals("==")) ns.push(new ValNeqTransNode(p1,p2));
                else if(cmpop.equals("!=")) ns.push(new ValEqTransNode(p1,p2));
				else
				{
					System.out.print("Unknown if '"+cmpop+"' ");
					break;
				}
				ns.push(new JzNode(target(m, ins.val)));

//                if (o.name.equals("ifle")) {
//                    String labelNo = c.getLabel();
//                    ValGtNode gtNode = new ValGtNode(ns.pop(), new IntConstNode(0));
//                    IfTransNode ifNode = new IfTransNode(gtNode, labelNo);
//                    int target = ins.val;
//                }
//                else if (o.name.equals("ifge")) {
//                    break;  // not supported yet
//                }
            }
			return;

		case IFCMP:			// if_icmplt, if_acmpne, ...
			d.print("\tif (" + stack2nd(ins) +o.opr +stacktop(ins) + ")\n\t\t");
			gengoto(d, m, ins, ins.val);

			{
				//Node go = new JumpNode(target(m, ins.val));

                Node p2 = ns.pop();
                Node p1 = ns.pop();

//				Node p1 = ns.pop();
//				Node p2 = ns.pop();

				String cmpop = o.opr.trim();

				// Reverse op! We use JZ, not JNZ!
				if(cmpop.equals("<="))		ns.push(new ValGtTransNode(p1,p2));
				else if(cmpop.equals("<"))		ns.push(new ValGeTransNode(p1,p2));
				else if(cmpop.equals(">="))		ns.push(new ValLtTransNode(p1,p2));
				else if(cmpop.equals(">"))		ns.push(new ValLeTransNode(p1,p2));
                else if(cmpop.equals("=="))		ns.push(new ValNeqTransNode(p1,p2));
                else if(cmpop.equals("!="))		ns.push(new ValEqTransNode(p1,p2));
				else
				{
					System.out.print("Unknown if '"+cmpop+"' ");
					break;
				}
                ns.push(new JzNode(target(m, ins.val)));

			}
			return;

		case TBLSW:			// [tableswitch]
// toba [
//			n = ins.more[1] - 3;		// correction factor
//			d.println("\tswitch (" + stacktop(ins) + ") {");
//			for (int i = 3; i < ins.more.length; i++) {
//				d.print("\t\tcase " + (i + n) + ": \t");
//				gengoto(d, m, ins, ins.more[i]);
//			}
//			d.print("\t\tdefault:\t");
//			gengoto(d, m, ins, ins.more[0]);
//			d.println("\t}");
// toba ]

            {
                Node p1 = ns.pop();
                n = ins.more[1] - 3;		// correction factor
                for (int i = 3; i < ins.more.length; i++) {
                    Node p2 = new IntConstNode(i + n);

                    ns.push(new ValNeqTransNode(p1,p2));
                    ns.push(new JzNode(target(m, ins.more[i])));
                }
                // default
                ns.push(new JumpNode(target(m, ins.more[0])));

                return;
            }

		case LKPSW:			// [lookupswitch]
// toba [
//			d.println("\tswitch (" + stacktop(ins) + ") {");
//			for (int i = 2; i < ins.more.length; i += 2) {
//				d.print("\t\tcase " + ins.more[i] + ": \t");
//				gengoto(d, m, ins, ins.more[i + 1]);
//			}
//			d.print("\t\tdefault:\t");
//			gengoto(d, m, ins, ins.more[0]);
//			d.println("\t}");
// toba ]

            {
                Node p1 = ns.pop();
                for (int i = 2; i < ins.more.length; i += 2) {
                    Node p2 = new IntConstNode(ins.more[i]);

                    ns.push(new ValNeqTransNode(p1,p2));
                    ns.push(new JzNode(target(m, ins.more[i + 1])));
                }
                // default
                ns.push(new JumpNode(target(m, ins.more[0])));

                return;
            }
		case GOTO:			// goto
			d.print("\t");
			gengoto(d, m, ins, ins.val);
			
			ns.push(new JumpNode(target(m, ins.val)));
			
			return;

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

            { // method call
                MethodNode methNode = new MethodNode(r.name);

                CallArgNode firstArgNode = null, nextArgNode = null;
                int argCount = getMethodArgCount(r.signature);
                for (int i=0; i<argCount; i++) {
                    Node arg = ns.pop();
                    if (firstArgNode == null) {
                        firstArgNode = new CallArgNode(arg, null);
                    } else {
                        firstArgNode = new CallArgNode(arg, nextArgNode);
                    }
                    nextArgNode = firstArgNode;
                }

//                CallArgNode firstArgNode = null, nextArgNode = null;
//                int argCount = getMethodArgCount(r.signature);
//                for (int i=0; i<argCount; i++) {
//                    Node arg = ns.pop();
//                    if (firstArgNode == null) {
//                        firstArgNode = new CallArgNode(arg, null);
//                        nextArgNode = firstArgNode;
//                    } else {
//                        CallArgNode argNode = new CallArgNode(arg, null);
//                        nextArgNode.setRight(argNode);
//                        nextArgNode = argNode;
//                    }
//                }
                IdentTransNode obj = (IdentTransNode)ns.pop();
//                if (ins.con.value instanceof MethodRef) {
                    String type = ((MethodRef)ins.con.value).cl.name;
                    type = "."+type;
                    PhantomType phantomType = new PhantomType(ClassMap.get_map().get(type, false, null));
                    obj.setType(phantomType);

                    PhantomStackVar pStackVar = ps.get_method().svars.get_var(obj.get_name());
                    pStackVar.setType(phantomType);
//                }
                OpMethodCallNode callNode = new OpMethodCallNode(obj, methNode, firstArgNode);
                ns.push(callNode);

                // change return type
                int index = m.fl.signature.indexOf(')');
                switch (m.fl.signature.charAt(index+1)) {
                    case 'I':
                        // todo setType
//                        methNode.setType(new PhantomType(ClassMap.get_map().get(".internal.int",false)));
                        break;
                    case 'Z':
                    case 'B':
                    case 'S':
                    case 'C':
                    case 'J':
                    case 'F':
                    case 'D':
                    case 'L':
                    case '[':
                    case 'V':
                    default:
                        break;
                }
                return;
            }

		case INONV:			// invokenonvirtual
			r = (FieldRef)ins.con.value;
			checkref(d, ins, argindex(ins, r.signature) + 1);
			invoke(d, ins, (MethodRef)r, 
					Names.hashmethod(r.name,r.cl.name,r.signature));
            { // constructor not supported yet (now insert drop opcode)
                Node dupSrc = ns.pop();
                Node dupDest = ns.pop();
                ns.push(new SequenceNode(dupSrc, new VoidNode(new EmptyNode())));
//                ns.push(dupSrc.getLeft());
                return;
//                if (ins.con.value instanceof MethodRef) {
//                    String type = ((MethodRef)ins.con.value).cl.name;
//                    type = "."+type;
//                    setTypeAutoVariable(ps, type);
//                    c = (ClassRef)ins.con.value;
//                    PhantomClass pc = ClassMap.get_map().get(c.name,false);
//                    PhantomType type = new PhantomType(pc);

//                    IdentNode node = new IdentNode();
//                    node. (IdentNode) ns.pop();
//                }
            }

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

            ns.push(new ReturnNode(ns.pop()));

            {   // update method return type
                switch (ident.charAt(0)) {
                    case 'i':
                        ps.getMethod().type = new PhantomType(ClassMap.get_map().get(".internal.int",false, null));
                        break;
                    case 'l':
                    case 'f':
                    case 'd':
                    case 'a':
                        if (m.fl.signature.endsWith(")Ljava/lang/String;")){
                            ps.getMethod().type = new PhantomType(ClassMap.get_map().get(".internal.string",false, null));
                        } else {
                            PhantomType returnType = getReturnType(m.fl.signature);
                            if (returnType != null) ps.getMethod().type = returnType;
                        }
                        break;
                    case 'r':
                    default:
                        break;
                }
            }

			return;

		case RETRN:			// return
			if (MethGen.needSwitch(m))
				d.println("\tgoto RETURN;");
			else
				d.println("\treturn;");

			ns.push(new ReturnNode(ns.pop()));

			return;

		case THROW:			// athrow
			d.println("\tathrow(" + stacktop(ins) + ");");

			ns.push(new ThrowNode(ns.pop()));

			return;

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
			throw new VerifyError("unimplemented opcode " + o.name + " at pc=" + ins.pc);

		}

		System.out.println("unimplemented opcode " + o.name + " at pc=" + ins.pc);

	}



	private static void ensureAutoVariable(ParseState ps, String ident) throws PlcException {
		if(ps.get_method().svars.get_var(ident) == null) {
//			ps.get_method().svars.add_stack_var(new PhantomVariable(ident, new PhantomType(ClassMap.get_map().get(".internal.object",false,null))));
            ps.get_method().svars.add_stack_var(new PhantomVariable(ident, new PhTypeObject()));
        }
    }

//    private static void setTypeStackVariable(ParseState ps, String ident, PhantomType type) throws PlcException {
//        if (type instanceof PhTypeInt && ps.get_method().isvars.get_var(ident) == null) {
//            ps.get_method().isvars.add_stack_var(new PhantomVariable(ident, type));
//        }
//        else if (ps.get_method().svars.get_var(ident) == null){
//            ps.get_method().svars.add_stack_var(new PhantomVariable(ident, type));
//        }
//    }
    private static void updateArrayTypeStackVariable(ParseState ps, String ident, PhantomType type) throws PlcException {
        PhantomStackVar stackVar = ps.get_method().svars.get_var(ident);
        if (stackVar != null){
            stackVar.setType(type);
        }
    }

//    private static void setTypeAutoVariable(ParseState ps, String type) throws PlcException {
//        PhantomStackVar stackVar = ps.get_method().svars.getLastVariable();
//        if(stackVar != null) {
//            (stackVar).setType(new PhantomType(ClassMap.get_map().get(type, false)));
//        }
//    }

    private static int getMethodArgCount(String signature) {
        int beginIndex = signature.indexOf('(');
        int endIndex = signature.indexOf(')');
        StringBuffer args = new StringBuffer(signature.substring(beginIndex+1, endIndex));

        int argCount = 0;
        if (args.length()==0) return 0;
        for (;args.length()>0;) {
            switch (args.charAt(0)) {
                case 'Z':
                case 'B':
                case 'S':
                case 'C':
                case 'I':
                case 'J':
                case 'F':
                case 'D':
                    argCount++;
                    args.delete(0,1);
                    break;
                case 'L':
                    argCount++;
                    int end = args.indexOf(";");
                    args.delete(0,end+1);
                    break;
                case '[':
                    args.delete(0,1);
                    break;
//                case 'V':
//                default:
//                    break;
            }
        }
        return argCount;
    }

    private static PhantomType getReturnType(String signature) throws PlcException{
        int index = signature.indexOf(")");
        String retType = signature.substring(index+1);
        return getFieldType(retType);

//        switch (retType.charAt(0)) {
//            case 'I':
//                return new PhantomType(ClassMap.get_map().get(".internal.int",false));
////                break;
//            case 'Z':
//            case 'B':
//            case 'S':
//            case 'C':
//            case 'J':
//            case 'F':
//            case 'D':
//                System.out.println("unimplemented return type " + retType.charAt(0));
//                return new PhantomType(ClassMap.get_map().get(".internal.object",false));
////                break;
//            case 'L':
////                int end = retType.indexOf(";");
//                String type = retType;
//
//                if (type.equals("Ljava/lang/String;")){
//                    return new PhantomType(ClassMap.get_map().get(".internal.string",false));
//                } else {
//                    type = "." + type.substring(1, type.length()-1).replace("/", ".");
//                    return new PhantomType(ClassMap.get_map().get(type, false));
//                }
////                break;
//            case '[':
//                retType = retType.substring(1);
//                System.out.println("unimplemented return type [");
//            case 'V':
//                break;
//
////                default:
////                    break;
//        }
//        return null;
    }

    public static PhantomType getFieldType(String signature) throws PlcException {
        switch (signature.charAt(0)) {
            case 'I':
                return new PhantomType(ClassMap.get_map().get(".internal.int",false, null));
            case 'Z':
            case 'B':
            case 'S':
            case 'C':
            case 'J':
            case 'F':
            case 'D':
                System.out.println("unimplemented type " + signature.charAt(0));
                return new PhantomType(ClassMap.get_map().get(".internal.object",false, null));
            case 'L':
                String type = signature;

                PhantomType ptype = null;
                if (type.equals("Ljava/lang/String;")){
                    ptype = new PhantomType(ClassMap.get_map().get(".internal.string",false, null));
                } else {
                    type = "." + type.substring(1, type.length()-1).replace("/", ".");
                    ptype = new PhantomType(ClassMap.get_map().get(type, false, null));
                }
                return ptype;
            case '[':
                String arrayType = signature.substring(1);
                System.out.println("unimplemented type [");
            case 'V':
                break;
            default:
                System.out.println("unknown type "+signature);
                break;
        }
        return null;
    }

    public static PhantomType getVoidArrayType() {
//                PhantomType arrayType = new PhantomType(ClassMap.get_map().get(".internal.void",false, null), true);
        PhantomType arrayType = new PhTypeVoid();
        arrayType.set_is_container(true);
        return arrayType;
    }

    public static PhantomType getArrayType(String operationName) throws PlcException {
        switch (operationName.charAt(0)) {
            case 'i':
                return new PhantomType(ClassMap.get_map().get(".internal.int",false, null), true);
            case 'a':
                return new PhantomType(ClassMap.get_map().get(".internal.object",false, null), true);
            case 'b': // byte or boolean
            case 'c': // char
            case 'd': // double
            case 'f': // float
            case 'l': // long
            case 's': // short
            default:
                System.out.println("unknown array type "+operationName);
                break;
        }
        return null;
    }

    public static PhantomType getPhantomType(String javaType) throws PlcException {
        if ("java.lang.String".equals(javaType)) return new PhTypeString();
        else if ("int".equals(javaType)) return new PhTypeInt();
        else return new PhTypeObject();
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
