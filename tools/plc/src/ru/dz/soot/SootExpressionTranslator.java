package ru.dz.soot;

import ru.dz.plc.compiler.ClassMap;
import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.binode.BiNode;
import ru.dz.plc.compiler.binode.NewNode;
import ru.dz.plc.compiler.binode.OpAndNode;
import ru.dz.plc.compiler.binode.OpDivideNode;
import ru.dz.plc.compiler.binode.OpMinusNode;
import ru.dz.plc.compiler.binode.OpMultiplyNode;
import ru.dz.plc.compiler.binode.OpOrNode;
import ru.dz.plc.compiler.binode.OpPlusNode;
import ru.dz.plc.compiler.binode.OpRemainderNode;
import ru.dz.plc.compiler.binode.OpShiftLeftNode;
import ru.dz.plc.compiler.binode.OpShiftRightNode;
import ru.dz.plc.compiler.binode.OpShiftRightUnsignedNode;
import ru.dz.plc.compiler.binode.OpSubscriptNode;
import ru.dz.plc.compiler.binode.OpXorNode;
import ru.dz.plc.compiler.binode.ValEqNode;
import ru.dz.plc.compiler.binode.ValGeNode;
import ru.dz.plc.compiler.binode.ValGtNode;
import ru.dz.plc.compiler.binode.ValLeNode;
import ru.dz.plc.compiler.binode.ValLtNode;
import ru.dz.plc.compiler.binode.ValNeqNode;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.InstanceOfNode;
import ru.dz.plc.compiler.node.IntConst64Node;
import ru.dz.plc.compiler.node.IntConstNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.NullNode;
import ru.dz.plc.compiler.node.OpArrayLength;
import ru.dz.plc.compiler.node.StaticLoadNode;
import ru.dz.plc.compiler.node.StringConstPoolNode;
import ru.dz.plc.compiler.node.SummonClassNode;
import ru.dz.plc.util.PlcException;
import soot.Local;
import soot.SootClass;
import soot.SootFieldRef;
import soot.Type;
import soot.Value;
import soot.jimple.AddExpr;
import soot.jimple.AndExpr;
import soot.jimple.ArrayRef;
import soot.jimple.CastExpr;
import soot.jimple.CaughtExceptionRef;
import soot.jimple.ClassConstant;
import soot.jimple.CmpExpr;
import soot.jimple.CmpgExpr;
import soot.jimple.CmplExpr;
import soot.jimple.DivExpr;
import soot.jimple.DoubleConstant;
import soot.jimple.DynamicInvokeExpr;
import soot.jimple.EqExpr;
import soot.jimple.FloatConstant;
import soot.jimple.GeExpr;
import soot.jimple.GtExpr;
import soot.jimple.InstanceFieldRef;
import soot.jimple.InstanceOfExpr;
import soot.jimple.IntConstant;
import soot.jimple.InterfaceInvokeExpr;
import soot.jimple.JimpleValueSwitch;
import soot.jimple.LeExpr;
import soot.jimple.LengthExpr;
import soot.jimple.LongConstant;
import soot.jimple.LtExpr;
import soot.jimple.MulExpr;
import soot.jimple.NeExpr;
import soot.jimple.NegExpr;
import soot.jimple.NewArrayExpr;
import soot.jimple.NewExpr;
import soot.jimple.NewMultiArrayExpr;
import soot.jimple.NullConstant;
import soot.jimple.OrExpr;
import soot.jimple.ParameterRef;
import soot.jimple.RemExpr;
import soot.jimple.ShlExpr;
import soot.jimple.ShrExpr;
import soot.jimple.SpecialInvokeExpr;
import soot.jimple.StaticFieldRef;
import soot.jimple.StaticInvokeExpr;
import soot.jimple.StringConstant;
import soot.jimple.SubExpr;
import soot.jimple.ThisRef;
import soot.jimple.UshrExpr;
import soot.jimple.VirtualInvokeExpr;
import soot.jimple.XorExpr;
import soot.jimple.internal.AbstractBinopExpr;

public class SootExpressionTranslator {

	private Value root;
	private Method m;
	private PhantomClass phantomClass;
	private ParseState ps;

	public SootExpressionTranslator(Value v, Method m, PhantomClass c, ParseState ps) {
		this.root = v;
		this.m = m;
		phantomClass = c;
	}

	public PhantomCodeWrapper process() throws PlcException
	{
		PhantomCodeWrapper ret = doValue(root);
		return ret;
	}

	public static PhantomType convertType( String tn ) throws PlcException
	{
		if( tn.equals("java.lang.Object")) tn = ".internal.object";
		if( tn.equals("java.lang.String")) tn = ".internal.string";
		if( tn.equals("java.lang.Integer")) tn = ".internal.int";
		
		if( tn.equals("int")) tn = ".internal.int";
		if( tn.equals("boolean")) tn = ".internal.int";

		if( tn.equals("byte")) tn = ".internal.int";
		if( tn.equals("short")) tn = ".internal.int";
		if( tn.equals("long")) tn = ".internal.long";
		
		boolean err = false;
		
		if( tn.equals("long")) { tn = ".internal.long"; err = true; }
		if( tn.equals("float")) { tn = ".internal.float"; err = true; }
		if( tn.equals("double")) { tn = ".internal.double"; err = true; }
		
		if( err ) SootMain.error("no type"+tn);

		if( tn.charAt(0) != '.' )
			tn = "."+tn;
		
		PhantomClass pc = ClassMap.get_map().get(tn, false, null);
		
		//PhantomClass pc = new PhantomClass(tn);
		if( pc == null )
			pc = new PhantomClass(tn);
		
		PhantomType pt = new PhantomType(pc);
		return pt;
	}
	
	public static PhantomType convertType( Type t ) throws PlcException
	{
		//SootMain.say(" ----------------- "+t+" tt "+t.getClass()+" n "+t.getNumber());

		//SootMain.say("conv type "+t.toString());
		
		PhantomType type = null;
		
		if (t instanceof soot.ArrayType) {
			soot.ArrayType at = (soot.ArrayType) t;
			
			if( at.numDimensions != 1)
				SootMain.error("multidim type "+at);
			
			type = convertType(at.baseType.toString());			
			//type.set_is_container(true);
			type = type.toContainer();
		}
		else
		{
			type = convertType(t.toString());
		}
		
		//SootMain.say("conv type "+t.toString()+" to "+type);
		
		return type;
	}
	
	private PhantomCodeWrapper doValue(final Value vv) throws PlcException 
	{
		final ww ret = new ww();
		ret.w = null;

		//SootMain.say("e    "+vv.toString());
		
		vv.apply(new JimpleValueSwitch() {

			@Override
			public void defaultCase(Object arg0) {
				SootMain.error("e ?? "+vv.getClass().getName());
				SootMain.error("e    "+vv.toString());
			}


			@Override
			public void caseClassConstant(ClassConstant v) {
				SootMain.say("class const "+v.value);
				
				try {
					ret.w = new PhantomCodeWrapper(new SummonClassNode(convertType(v.value)));
				} catch (PlcException e) {
					SootMain.error(e);
				}
			}

			@Override
			public void caseDoubleConstant(DoubleConstant v) {
				ret.w = new PhantomCodeWrapper(new IntConst64Node(Double.doubleToRawLongBits(v.value)));				
			}

			@Override
			public void caseFloatConstant(FloatConstant v) {				
				ret.w = new PhantomCodeWrapper(new IntConstNode(Float.floatToIntBits(v.value)));
				
			}

			@Override
			public void caseIntConstant(IntConstant v) {
				ret.w = new PhantomCodeWrapper(new IntConstNode(v.value));
			}

			@Override
			public void caseLongConstant(LongConstant v) {
				ret.w = new PhantomCodeWrapper(new IntConst64Node(v.value));				
			}

			@Override
			public void caseNullConstant(NullConstant arg0) {
				ret.w = new PhantomCodeWrapper(new NullNode());
			}

			@Override
			public void caseStringConstant(StringConstant v) {
				ret.w = doStringConst(v);				
			}

			@Override
			public void caseAddExpr(AddExpr v) {
				ret.w = new BinOpWrapper<OpPlusNode>() {@Override
					OpPlusNode create(Node l, Node r) { return new OpPlusNode(l,r); }} .doBinOp(v);
			}

			// TODO logical?
			@Override
			public void caseAndExpr(AndExpr v) {
				ret.w = new BinOpWrapper<OpAndNode>() {@Override
					OpAndNode create(Node l, Node r) {				
						return new OpAndNode(l,r);
					}} .doBinOp(v);
			}

			@Override
			public void caseCastExpr(CastExpr v) {
				// TODO Auto-generated method stub
				SootMain.error(v.toString());				
			}

			@Override
			public void caseCmpExpr(CmpExpr v) {
				// TODO Auto-generated method stub
				SootMain.error(v.toString());				
			}

			@Override
			public void caseCmpgExpr(CmpgExpr v) {
				// TODO Auto-generated method stub
				SootMain.error(v.toString());				
			}

			@Override
			public void caseCmplExpr(CmplExpr v) {
				// TODO Auto-generated method stub
				SootMain.error(v.toString());
			}

			@Override
			public void caseDivExpr(DivExpr v) {
				ret.w = new BinOpWrapper<OpDivideNode>() {@Override
					OpDivideNode create(Node l, Node r) { return new OpDivideNode(l,r); }} .doBinOp(v);
			}

			@Override
			public void caseDynamicInvokeExpr(DynamicInvokeExpr v) {
				try {
					ret.w = PhantomCodeWrapper.getInvoke(v, m, phantomClass, ps );
				} catch (PlcException e) {
					SootMain.error(e);
				}
			}

			@Override
			public void caseEqExpr(EqExpr v) {
				//v.getType()
				
				ret.w = new BinOpWrapper<ValEqNode>() {@Override
					ValEqNode create(Node l, Node r) throws PlcException { return new ValEqNode(l,r); }} .doBinOp(v);
				
				//ret.w = new BinOpWrapper<RefEqNode>() {@Override
				//	RefEqNode create(Node l, Node r) { return new RefEqNode(l,r); }} .doBinOp(v);
			}

			@Override
			public void caseGeExpr(GeExpr v) {
				ret.w = new BinOpWrapper<ValGeNode>() {@Override
					ValGeNode create(Node l, Node r) { return new ValGeNode(l,r); }} .doBinOp(v);
			}

			@Override
			public void caseGtExpr(GtExpr v) {
				ret.w = new BinOpWrapper<ValGtNode>() {@Override
					ValGtNode create(Node l, Node r) { return new ValGtNode(l,r); }} .doBinOp(v);
			}

			@Override
			public void caseInstanceOfExpr(InstanceOfExpr v) {
				try {
					PhantomType phantomType = convertType(v.getCheckType());
					Node expr = PhantomCodeWrapper.getExpression(v.getOp(), m, phantomClass, ps).getNode();
					InstanceOfNode node = new InstanceOfNode(expr,phantomType);
					ret.w = new PhantomCodeWrapper(node);
				} catch (PlcException e) {
					SootMain.error(e);
				}
				
			}

			@Override
			public void caseInterfaceInvokeExpr(InterfaceInvokeExpr v) {
				try {
					// TODO right?
					ret.w = PhantomCodeWrapper.getInvoke(v, m, phantomClass, ps );
				} catch (PlcException e) {
					SootMain.error(e);
				}
			}

			@Override
			public void caseLeExpr(LeExpr v) {
				ret.w = new BinOpWrapper<ValLeNode>() {@Override
					ValLeNode create(Node l, Node r) { return new ValLeNode(l,r); }} .doBinOp(v);
			}

			@Override
			public void caseLengthExpr(LengthExpr v) {
				try {
					ret.w = doLength(v);
				} catch (PlcException e) {
					SootMain.error(e);
				}
			}

			@Override
			public void caseLtExpr(LtExpr v) {
				ret.w = new BinOpWrapper<ValLtNode>() {@Override
					ValLtNode create(Node l, Node r) { return new ValLtNode(l,r); }} .doBinOp(v);
			}

			@Override
			public void caseMulExpr(MulExpr v) {
				ret.w = new BinOpWrapper<OpMultiplyNode>() {@Override
					OpMultiplyNode create(Node l, Node r) { return new OpMultiplyNode(l,r); }} .doBinOp(v);
			}

			@Override
			public void caseNeExpr(NeExpr v) {
				//ret.w = new BinOpWrapper<RefNeqNode>() {@Override
				//	RefNeqNode create(Node l, Node r) { return new RefNeqNode(l,r); }} .doBinOp(v);
				ret.w = new BinOpWrapper<ValNeqNode>() {@Override
					ValNeqNode create(Node l, Node r) throws PlcException { return new ValNeqNode(l,r); }} .doBinOp(v);
			}

			@Override
			public void caseNegExpr(NegExpr v) {
				// TODO Auto-generated method stub


				//Node expr = PhantomCodeWrapper.getExpression(v.getOp(), m, phantomClass).getNode();
				//InstanceOfNode node = new ?Node(expr,phantomType);
				//ret.w = new PhantomCodeWrapper(node);
				
				
				SootMain.error(v.toString());
			}

			@Override
			public void caseNewArrayExpr(NewArrayExpr v) {
				// TODO calc size expr?
				//v.getSize();
				Type type = v.getType();
				SootMain.say("array type "+type);
				doNew(ret, type); // Right?
			}

			@Override
			public void caseNewExpr(NewExpr v) {
				doNew(ret, v.getType());
			}

			@Override
			public void caseNewMultiArrayExpr(NewMultiArrayExpr v) {
				SootMain.error(v.toString()); 
				
				int dimensions = v.getSizeCount();
				for(int dim = 0; dim < dimensions; dim++)
				{
					Value size = v.getSize(dim); // Array dimension?
					say("  dim = "+size );
				}

				// TODO New multi-array - try and check how Soot calcs array displacement. possibly its ok to make just a plain array 
				doNew(ret, v.getType()); // Right?
			}

			@Override
			public void caseOrExpr(OrExpr v) {
				ret.w = new BinOpWrapper<OpOrNode>() {@Override
					OpOrNode create(Node l, Node r) { return new OpOrNode(l,r); }} .doBinOp(v);
			}

			@Override
			public void caseRemExpr(RemExpr v) {
				ret.w = new BinOpWrapper<OpRemainderNode>() {@Override
					OpRemainderNode create(Node l, Node r) { return new OpRemainderNode(l,r); }} .doBinOp(v);
			}

			
			
			@Override
			public void caseShlExpr(ShlExpr v) {
				ret.w = new BinOpWrapper<OpShiftLeftNode>() {@Override
					OpShiftLeftNode create(Node l, Node r) { return new OpShiftLeftNode(l,r); }} .doBinOp(v);
			}

			@Override
			public void caseShrExpr(ShrExpr v) {
				ret.w = new BinOpWrapper<OpShiftRightNode>() {@Override
					OpShiftRightNode create(Node l, Node r) { return new OpShiftRightNode(l,r); }} .doBinOp(v);
			}

			@Override
			public void caseUshrExpr(UshrExpr v) { 
				ret.w = new BinOpWrapper<OpShiftRightUnsignedNode>() {@Override
					OpShiftRightUnsignedNode create(Node l, Node r) { return new OpShiftRightUnsignedNode(l,r); }} .doBinOp(v);
			}

			
			
			@Override
			public void caseSpecialInvokeExpr(SpecialInvokeExpr v) {
				try {
					ret.w = PhantomCodeWrapper.getInvoke(v, m, phantomClass, ps );
				} catch (PlcException e) {
					SootMain.error(e);
				}
			}

			@Override
			public void caseStaticInvokeExpr(StaticInvokeExpr v) {
				try {
					ret.w = PhantomCodeWrapper.getInvoke(v, m, phantomClass, ps );
				} catch (PlcException e) {
					SootMain.error(e);
				}
			}

			@Override
			public void caseSubExpr(SubExpr v) {
				ret.w = new BinOpWrapper<OpMinusNode>() {@Override
					OpMinusNode create(Node l, Node r) { return new OpMinusNode(l,r); }} .doBinOp(v);
			}

			@Override
			public void caseXorExpr(XorExpr v) {
				ret.w = new BinOpWrapper<OpXorNode>() {@Override
					OpXorNode create(Node l, Node r) { return new OpXorNode(l,r); }} .doBinOp(v);
			}


			
			@Override
			public void caseVirtualInvokeExpr(VirtualInvokeExpr v) {
				try {
					ret.w = PhantomCodeWrapper.getInvoke(v, m, phantomClass, ps );
				} catch (PlcException e) {
					SootMain.error(e);
				}
			}

			@Override
			public void caseArrayRef(ArrayRef v) {
				try {
					ret.w = doArrayRef(v);
				} catch (PlcException e) {
					SootMain.error(e);
				}
				
			}

			@Override
			public void caseCaughtExceptionRef(CaughtExceptionRef v) {
				// TODO Auto-generated method stub
				SootMain.error(v.toString());				
			}

			@Override
			public void caseInstanceFieldRef(InstanceFieldRef v) {
				String varName = v.getField().getName();
				Value base = v.getBase();
				//SootMain.say("field "+varName+" base = "+base);

				PhantomCodeWrapper b;
				try {
					b = doValue(base);
					//SootMain.say("code "+b+" node = "+b.getNode());
				} catch (PlcException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
					b = null;
				}				
				IdentNode node = new IdentNode( b.getNode(), varName, ps ); // IdentNode automatically looks for for field or stack var by name
				
				//IdentNode node = new IdentNode( varName ); // IdentNode automatically looks for for field or stack var by name
				ret.w = new PhantomCodeWrapper( node );				
			}

			@Override
			public void caseParameterRef(ParameterRef v) {
				/*
				int parameterIndex = v.getIndex();
				String varName = v.getField().getName();
				IdentNode node = new IdentNode( varName ); // IdentNode automatically looks for for field or stack var by name
				ret.w = new PhantomCodeWrapper( node );
				*/
				// TODO make caseParameterRef
				SootMain.error(v.toString());
			}

			@Override
			public void caseStaticFieldRef(StaticFieldRef v) {
				SootFieldRef ref = v.getFieldRef();
				String varName = ref.name();
				
				SootClass declaringClass = ref.declaringClass();
				String className = declaringClass.getName();
				
				say("ref to static field '"+varName+"' from "+className);
				
				try {
					PhantomType phantomType = convertType(className);					
					Node n = new StaticLoadNode(phantomType.get_class(),varName);
					ret.w = new PhantomCodeWrapper( n );
				} catch (PlcException e) {
					SootMain.error(e);
				}
			}

			@Override
			public void caseThisRef(ThisRef arg0) {
				ret.w = new PhantomCodeWrapper(new ru.dz.plc.compiler.node.ThisNode(phantomClass));
			}

			@Override
			public void caseLocal(Local v) {
				ret.w = doReadLocal(v);				
			}
			
		});
		
		if(ret.w != null)
			return ret.w;
/*		
		SootMain.say("type unimpl "+v.getClass());
		
		if( v instanceof StringConstant )
			return doStringConst((StringConstant)v);

		if( v instanceof JimpleLocal )
			return doReadLocal((JimpleLocal)v);

		if( v instanceof JAddExpr )
			return doAdd((JAddExpr)v);
		
		if( v instanceof JSubExpr )
			return doSub((JSubExpr)v);
		
		if( v instanceof JMulExpr )
			return new BinOpWrapper<OpMultiplyNode>() {@Override
			OpMultiplyNode create(Node l, Node r) {				
				return new OpMultiplyNode(l,r);
			}} .doBinOp(v);
		
		if( v instanceof IntConstant )
			return new PhantomCodeWrapper( new IntConstNode(((IntConstant)v).value));
		
		if( v instanceof JArrayRef )
			return doArrayRef((JArrayRef)v);
		
		
		if( v instanceof JLengthExpr )
			return doLength((JLengthExpr)v);
		

		if( v instanceof JVirtualInvokeExpr )
			return doVirtualInvoke((JVirtualInvokeExpr)v);
		
		if( v instanceof JStaticInvokeExpr )
			return doStaticInvoke((JStaticInvokeExpr)v);
*/
		
		say("e ?? "+vv.getClass().getName());
		say("e    "+vv.toString());
		return PhantomCodeWrapper.getNullNode();
	}


	

	private PhantomCodeWrapper doStringConst(StringConstant v) {
		//return new PhantomCodeWrapper(new StringConstNode(v.value));
		return new PhantomCodeWrapper(new StringConstPoolNode(v.value,phantomClass));
	}

	private PhantomCodeWrapper doLength(LengthExpr v) throws PlcException {
		Value array = v.getOp();
		
		return new PhantomCodeWrapper(new OpArrayLength(PhantomCodeWrapper.getExpression(array, m, phantomClass, ps).getNode()));
	}

	/*
	private PhantomCodeWrapper doStaticInvoke(StaticInvokeExpr v) {
		SootMethodRef mr = v.getMethodRef();
		String mName = mr.name();
		mr.declaringClass();
		say("Static call "+mName);
		// TO DO static call
		return new PhantomCodeWrapper(new NullNode());
	}

	private PhantomCodeWrapper doVirtualInvoke(VirtualInvokeExpr v) throws PlcException {
		//SootMethodRef mr = v.getMethodRef();
		//String mName = mr.name();
		//say("Virtual call "+mName);
		// TO DO virt call
		//return new PhantomCodeWrapper(new NullNode());
		
		return PhantomCodeWrapper.getInvoke(v, m, phantomClass );

	}
	*/
	
	private PhantomCodeWrapper doArrayRef(ArrayRef v) throws PlcException {
		Value base = v.getBase();
		Value index = v.getIndex();
		
		Node array = doValue(base).getNode();
		Node subscr = doValue(index).getNode();
		
		OpSubscriptNode subscriptNode = new OpSubscriptNode(array,subscr);
		return new PhantomCodeWrapper(subscriptNode);
	}

	/*
	private PhantomCodeWrapper doAdd(JAddExpr v) throws PlcException {
		Type t = v.getType();
		assertInt(t);

		Value e1 = v.getOp1();
		Value e2 = v.getOp2();
		
		Node e1n = doValue(e1).getNode();
		Node e2n = doValue(e2).getNode();
		
		OpPlusNode node = new OpPlusNode(e1n,e2n);
		node.setType(convertType(t));
		
		return new PhantomCodeWrapper( node );
	}
	
	private PhantomCodeWrapper doSub(JSubExpr v) throws PlcException {
		Type t = v.getType();
		assertInt(t);
		
		Value e1 = v.getOp1();
		Value e2 = v.getOp2();
		
		Node e1n = doValue(e1).getNode();
		Node e2n = doValue(e2).getNode();
		
		OpMinusNode node = new OpMinusNode(e1n,e2n);
		node.setType(convertType(t));
		return new PhantomCodeWrapper( node);
	}
	*/
	
	/*
	private void assertInt(Type t) {
		if( t.toString().equals("int"))
			return;
		//Type machineType = Type.toMachineType(t);
		say("unknown type "+t);
		//say("t1 "+machineType);
		throw new RuntimeException("Unknown type "+t);
	}
	*/
	
	private PhantomCodeWrapper doReadLocal(Local v) {
		String varName = v.getName();
		//SootMain.say("read local '"+varName+"'");
		IdentNode node = new IdentNode( varName, ps );
		return new PhantomCodeWrapper( node );
	}

	private void say(String string) {
		System.err.println(string);
	}

	
	
	private void doNew(final ww ret, Type type) {
		try {
			PhantomType phantomType = convertType(type);
			ret.w = new PhantomCodeWrapper( new NewNode(phantomType, null, null ) );				
		} catch (PlcException e) {
			SootMain.error(e);
		}
	}



	abstract class BinOpWrapper<T extends BiNode>
	{
		
		abstract T create( Node l, Node r ) throws PlcException;
		
		PhantomCodeWrapper doBinOp(Value _v) {
			AbstractBinopExpr v = (AbstractBinopExpr) _v;
			Type t = v.getType();

			Value e1 = v.getOp1();
			Value e2 = v.getOp2();
			
			try {
				Node e1n = doValue(e1).getNode();
				Node e2n = doValue(e2).getNode();

				Node n = create(e1n,e2n);
				n.presetType(SootExpressionTranslator.convertType(t));
				return new PhantomCodeWrapper( n );
			} catch (PlcException e) {
				SootMain.error("Exception "+e);
				return new PhantomCodeWrapper( new NullNode() );
			}
			
			
		}		
		
	}
	
}


