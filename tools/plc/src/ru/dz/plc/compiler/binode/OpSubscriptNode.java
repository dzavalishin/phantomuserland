package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.PhTypeUnknown;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Array subscription node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class OpSubscriptNode extends BiNode {
	public OpSubscriptNode(Node atom, Node subscr) {    super(atom,subscr);  }
	public String toString()  {    return "[]";  }

	public PhantomType find_out_my_type() throws PlcException
	{
		Node atom = _l;

		PhantomType a_type = atom.getType();

		if( a_type == null || a_type.is_unknown() )
			return new PhTypeUnknown();

		if( !a_type.is_container() )
			throw new PlcException( "[] Node", "not a container subscripted" );

		return new PhantomType( a_type.get_class() );		
	}

	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		Node atom = _l;
		Node subscr = _r;

		log.fine("Node "+this+" codegen");

		// put array object to load from
		atom.generate_code(c,s);
		move_between_stacks(c, atom);

		// put subscript
		subscr.generate_code(c,s);
		move_between_stacks(c, subscr);

		c.emitCall(10,1); // Method number 10, 1 parameter - get
	}

	protected void generateMyLlvmCode(ru.dz.plc.compiler.LlvmCodegen llc) throws PlcException 
	{
		Node atom = _l;
		Node subscr = _r;

		log.fine("Node "+this+" codegen");

		// put array object to load from
		atom.generateLlvmCode(llc);	
		//move_between_stacks(c, atom.is_on_int_stack());

		// put subscript
		subscr.generateLlvmCode(llc);
		//move_between_stacks(c, subscr.is_on_int_stack());

		//c.emitCall(10,1); // Method number 10, 1 parameter - get
		llc.putln(getLlvmTempName()+" = call "+LlvmCodegen.getObjectType()+" @PhantomVm_ArrayLoad( "+LlvmCodegen.getObjectType()+" "+_l.getLlvmTempName()+", i32 "+_r.getLlvmTempName()+ ") ;");
	}
	

}