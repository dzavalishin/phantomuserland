package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Generic value compare node.</p>
 * <p>Copyright: Copyright (c) 2004-2017 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public abstract class ValCmpNode extends BiBistackNode {

	protected String opName = "__undefined_in_subclass!!__";
	
	public ValCmpNode(Node l, Node r) 
	{
		super(l, r);
		// we're always int
		presetType(PhantomType.getInt());
	}

	@Override
	public boolean args_on_int_stack() {
		return true; // we allways need em on int stack
	}

	public PhantomType find_out_my_type() throws PlcException
	{
		return PhantomType.getInt();
	}	
	 
	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		
		if( _l.getType().is_on_int_stack() || _r.getType().is_on_int_stack() )
		{
			_l.generate_code(c, s);
			if (!_l.is_on_int_stack()) 			
			{
				//c.emit_o2i();
				move_between_stacks(c, _l.is_on_int_stack(), _l.getType());
			}
			
			_r.generate_code(c, s);
			if (!_r.is_on_int_stack())
			{
				//c.emit_o2i();
				move_between_stacks(c, _r.is_on_int_stack(), _r.getType());
			}
			
			log.fine("Node "+this+" codegen");
			generate_my_code(c,s);
		}
		else
			super.generate_code(c, s);
	}
	
	
	@Override
	public void generate_C_code(C_codegen cgen, CodeGeneratorState s) throws PlcException {

		// TODO move between stacks?
		
		cgen.put(C_codegen.getJitRuntimeFuncPrefix()+"valueCmp_"+opName+"( " );
		_l.generate_C_code(cgen, s);
		cgen.put(" ) "); 
		_r.generate_C_code(cgen, s);	
		cgen.put(" ) "); 
	}

	// We always return result on integer stack
	@Override
	public boolean is_on_int_stack() {
		return true;
	}
	
	/* not - eq/neq can compare other values *
	
	// We allways want params on int stack
	@Override
	public boolean args_on_int_stack() {
		return true;
	}
	*/
	
	
	protected void generateCmpOp( Codegen c, RunBinaryOp op  ) throws PlcException
	{
		if(common_type == null) throw new PlcException("cmp op","not preprocessed");

		try {
			c.emitNumericPrefix(common_type);
			op.run(); // generate actual op
		} catch (IOException e) {
			throw new PlcException("io error", e);
		}

	}

	
}