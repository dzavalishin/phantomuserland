package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomField;
import ru.dz.plc.compiler.PhantomStackVar;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.CastNode;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Assignment node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class OpAssignNode extends BiNode {
	
	/**
	 * Type of variable we assign to.
	 */
	private PhantomType varType;

	/**
	 * Variable/array assign node.
	 * 
	 * @param var_or_array where to assign
	 * @param expr expression to assign 
	 */
	public OpAssignNode(Node var_or_array, Node expr) {    
		super(var_or_array,expr);  
		}
	
	public String toString()  {    return "=";  }

	public boolean args_on_int_stack() {
		if( _l.getClass() == IdentNode.class )
		{
			return _l.args_on_int_stack(); // integer stack var will tell it has args on int stack
		}
		return false; 
	}

	@Override
	public boolean is_on_int_stack() {
		return args_on_int_stack();
	}

	/*
	@Override
	public void preprocess_me(ParseState s) throws PlcException 
	{
		//if(getType().is_void())			return;
		
		if( !_l.getType().equals(_r.getType()) )
			_r = new CastNode(_r, _l.getType());
	}*/
	
	
	private void generateCast(PhantomType varType) throws PlcException 
	{
		if( !varType.equals(_r.getType()) )
			_r = new CastNode(_r, varType);
	}
	

	@Override
	public PhantomType find_out_my_type() throws PlcException {
		// We return a copy of expression
		return _r.getType();
	}
	
	/**
	 * If caller is void, we return void too. See skipping DUP op in code generation.
	 */
	public PhantomType getType() throws PlcException
	{
		if(isVoidParent()) return PhantomType.getVoid();
		return super.getType();
	}
	
	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		generate_my_code(c,s);
	}

	// NB! We must return a copy of assigned value.
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {
		if( _l.getClass() == IdentNode.class )
			assignVarCode(c, s);
		else if( _l.getClass() == OpSubscriptNode.class )
			assignArrayCode(c, s);
		else
			throw new PlcException("= Node", "unknown left Node", _l.toString() );

	}

	public void assignArrayCode(Codegen c, CodeGeneratorState s) throws PlcException, IOException {
		if(!isVoidParent()) 
			throw new PlcException("Can't reuse array assign value, please simplify expression.");
			
		// this is assignment to array element
		OpSubscriptNode dest = (OpSubscriptNode)_l;

		Node atom = dest.getLeft();
		Node subscr = dest._r;

		PhantomType destType = new PhantomType( atom.getType().get_class() );

		//check_assignment_types("container element", type, _r.getType());
		check_assignment_types("container element", destType, _r.getType());
		generateCast(destType);

		// array object to assign to
		atom.generate_code(c,s);
		move_between_stacks(c, atom.is_on_int_stack(), atom.getType());

		// put value to assign
		if( _r != null ) 
		{ 
			_r.generate_code(c,s); 
			move_between_stacks(c, _r.is_on_int_stack(), _r.getType()); 
		}
		else System.out.println("OpAssignNode.generate_my_code() _r is null!"); // TODO die here

		// put subscript
		subscr.generate_code(c,s);
		move_between_stacks(c, subscr.is_on_int_stack(), subscr.getType());

		c.emitCall(11,2); // Method number 11, 2 parameters
		c.emitOsDrop(); // Method return
		
		// NB! Must return copy of assigned stuff! NB! Must increment refcount!
		// (currently does, make sure it will)
		if(is_on_int_stack()) System.out.println("OpAssignNode.generate_my_code() i'm on int??!");

	}

	public void assignVarCode(Codegen c, CodeGeneratorState s) throws IOException, PlcException {

		/*
		if( _r != null ) 
		{ 
			_r.generate_code(c,s); 
			move_between_stacks(c, _r.is_on_int_stack(), _r.getType()); 
		}*/

		IdentNode dest = (IdentNode) _l;
		String dest_name = dest.getName();

		// Field?
		PhantomField f = s.get_class().find_field(dest_name);
		if (f != null) {
			varType = f.getType();
			check_assignment_types(f.getName(), varType,_r.getType());
			generateCast( varType );

			_r.generate_code(c,s); 
			move_between_stacks(c, _r.is_on_int_stack(), _r.getType()); 
			
			if(!isVoidParent()) c.emitOsDup(); // return a copy
			c.emitComment("set "+dest_name);
			c.emitSave(f.getOrdinal());
			
			if(is_on_int_stack()) System.out.println("OpAssignNode.generate_my_code() i'm on int for field??!");
			return;
		}

		// Stack var?
		PhantomStackVar svar = s.istack_vars().get_var(dest_name);
		if (svar != null)
		{
			varType = svar.getType();
			check_assignment_types(svar.getName(), varType,_r.getType());
			generateCast( varType );

			_r.generate_code(c,s); 
			move_between_stacks(c, _r.is_on_int_stack(), _r.getType()); 
			
			if(!isVoidParent()) 
			{
				c.emitNumericPrefix(_r.getType());
				c.emitIsDup(); // return a copy
			}
			
			c.emitComment("set "+dest_name);
			c.emitNumericPrefix(_r.getType());
			c.emitISet(svar.get_abs_stack_pos()); // set stack variable
			
			if(!is_on_int_stack()) System.out.println("OpAssignNode.generate_my_code() i'm on obj??!");
			return;
		}
		
		svar = s.stack_vars().get_var(dest_name);
		if (svar != null)
		{
			varType = svar.getType();
			check_assignment_types(svar.getName(), varType,_r.getType());
			generateCast( varType );

			_r.generate_code(c,s); 
			move_between_stacks(c, _r.is_on_int_stack(), _r.getType()); 
			
			if(!isVoidParent()) c.emitOsDup(); // return a copy
			
			c.emitComment("set "+dest_name);
			c.emitSet(svar.get_abs_stack_pos()); // set stack variable
			if(is_on_int_stack()) System.out.println("OpAssignNode.generate_my_code() i'm on int??!");
			return;
		}

		throw new PlcException("= Node", "no field or var", dest_name);
	}

	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException {
		if( _l.getClass() == IdentNode.class )
		{
			if( _r == null )				throw new PlcException("null expr");
			
			//_r.generateLlvmCode(llc);

			IdentNode dest = (IdentNode) _l;
			String dest_name = dest.getName();

			// Field?
			PhantomField f = llc.getPhantomClass().find_field(dest_name);
			if (f != null) {
				//if (type == null || type.is_unknown()) type = f.getType();
				//check_assignment_types(f.getName(), type,_r.getType());
				//c.emitOsDup(); // return a copy
				//c.emitSave(f.getOrdinal());
				//if(is_on_int_stack()) System.out.println("OpAssignNode.generate_my_code() i'm on int??!");
				// TODO llvm
				llc.putln(String.format( "; @this.%s = %s", f.getName(), _r.getLlvmTempName() ));
				return;
			}

			// Stack var?
			PhantomStackVar svar = llc.getIstackVars().get_var(dest_name);
			if (svar != null)
			{
				//if (type == null || type.is_unknown()) type = svar.getType();
				//check_assignment_types(svar.getName(), type,_r.getType());
				//c.emitIsDup(); // return a copy
				//c.emitISet(svar.get_abs_stack_pos()); // set stack variable
				//if(!is_on_int_stack()) System.out.println("OpAssignNode.generate_my_code() i'm on obj??!");
				// TODO llvm
				llc.putln(String.format( "; @%s = %s", svar.getName(), _r.getLlvmTempName() ));
			}
			else
			{
				svar = llc.GetOstackVars().get_var(dest_name);
				if (svar == null)
					throw new PlcException(toString(), "nowhere to assign", dest_name);

				//if (type == null || type.is_unknown()) type = svar.getType();
				//check_assignment_types(svar.getName(), type,_r.getType());
				//c.emitOsDup(); // return a copy
				//c.emitSet(svar.get_abs_stack_pos()); // set stack variable
				//if(is_on_int_stack()) System.out.println("OpAssignNode.generate_my_code() i'm on int??!");
				// TODO llvm
				llc.putln(String.format( "; @%s = %s", svar.getName(), _r.getLlvmTempName() ));
			}
		}
		else if( _l.getClass() == OpSubscriptNode.class )
		{
			// this is assignment to array element
			OpSubscriptNode dest = (OpSubscriptNode)_l;

			Node atom = dest.getLeft();
			Node subscr = dest._r;


			// array object to assign to
			//atom.generateLlvmCode(llc);
			//move_between_stacks(c, atom.is_on_int_stack());

			// put value to assign
			if( _r == null ) throw new PlcException("nothing to assign");

			//_r.generateLlvmCode(llc);
			
			// put subscript
			//subscr.generateLlvmCode(llc);
			//move_between_stacks(c, subscr.is_on_int_stack());

			//c.emitCall(11,2); // Method number 11, 2 parameters
			// NB! Must return copy of assigned stuff! NB! Must increment refcount!
			// (currently does, make sure it will)
			//if(is_on_int_stack()) System.out.println("OpAssignNode.generate_my_code() i'm on int??!");

			PhantomType destType = new PhantomType( atom.getType().get_class() );

			//check_assignment_types("container element", type, _r.getType());
			check_assignment_types("container element", destType, _r.getType());

			llc.putln(String.format( "; %s[%s]  = %s", atom.getLlvmTempName(), subscr.getLlvmTempName(), _r.getLlvmTempName() ));
		}
		else
			throw new PlcException("= Node", "unknown left Node", _l.toString() );
	}
	
}
