package ru.dz.plc.compiler.binode;

import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.CastNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

public abstract class EqNeqNode extends BiBistackNode {

	public EqNeqNode(Node l, Node r) {
		super(l, r);
	}

	public PhantomType find_out_my_type() throws PlcException
	{
		return PhantomType.getInt();
	}	
	
	@Override
	public void preprocess_me(ParseState s) throws PlcException {
		//super.preprocess_me(s);

		common_type = PhantomType.findCompatibleType(_l.getType(),_r.getType());
		if( common_type == null )
		{
			// we can compare anything for we call method
			assert(go_to_object_stack());			
			return;
		}
		
		if( !_l.getType().equals(common_type) )
		{
			//System.out.println("cast "+_l.getType()+ " to "+common_type);
			_l = new CastNode(_l, common_type);
		}

		if( !_r.getType().equals(common_type) )
		{
			//System.out.println("cast "+_r.getType()+ " to "+common_type);
			_r = new CastNode(_r, common_type);
		}
	}

}
