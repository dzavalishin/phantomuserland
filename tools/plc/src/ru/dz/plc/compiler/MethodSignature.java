package ru.dz.plc.compiler;

import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

import ru.dz.plc.compiler.binode.CallArgNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Represents a signature of a method.</p>
 * <p>Used as a key into the method table.</p>
 * 
 * 
 * <p>Copyright: Copyright (c) 2004-2016 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * 
 * @author dz
 */



public class MethodSignature {

	private static final String SIGNATURE_DELIMITER = "_#_";
	private String signature;
	private List<PhantomType> args;
	private String name;

	public MethodSignature(String name, List<PhantomType> args) 
	{
		generateSignature(name, args);
	}

	public MethodSignature(String name, Node argsNodes) throws PlcException 
	{
		List<PhantomType> args = new LinkedList<PhantomType>();
		
		CallArgNode n = (CallArgNode) argsNodes;
		while( n!= null )
		{
			Node curr = n.getLeft();
			n = (CallArgNode) n.getRight();
			
			//curr.find_out_my_type();
			PhantomType type = curr.getType();
			
			args.add(type);
		}
		
		generateSignature(name, args);
	}

	public MethodSignature(String name, Iterator<ArgDefinition> argIterator) {
		List<PhantomType> args = new LinkedList<PhantomType>();
		while(  argIterator.hasNext() )
		{
			ArgDefinition ad = argIterator.next();
			args.add(ad.getType());
			
		}
		generateSignature(name, args);
	}

	private void generateSignature(String Name, List<PhantomType> args) {
		if( Name == null )
			Name = "(null)";
		
		name = Name;
		this.args = args;
		
		// Generate signature string
		StringBuilder sb = new StringBuilder(Name);
		
		for( PhantomType t : args )
		{
			sb.append(SIGNATURE_DELIMITER); // Some character that can't happen in type name
			sb.append(t.toString());
			assert(!t.is_void());
		}
		
		if( args.size() == 0 )
		{
			sb.append(SIGNATURE_DELIMITER);
			sb.append("void");
		}
		
		signature = sb.toString();
		//SootMain.say(signature); 
	}

	
	
	public String getSignature() {
		return signature;
	}

	@Override
	public boolean equals(Object obj) {
		if (obj instanceof MethodSignature) {
			MethodSignature him = (MethodSignature) obj;
			return signature.equals(him.signature);
		}
		return false;
	}

	@Override
	public int hashCode() {
		return signature.hashCode();
	}
	
	/**
	 * <p>If this signature describes method that can be called for parameters given signature describes.</p>
	 * <p>Practically, each parameter type must be identical or ancestor of corresponding parameter in our signature.</p>
	 * 
	 * @param callSignature Signature of what caller supposes to call.
	 * 
	 * @return True if passed signature describes parameters method having our signature can accept. 
	 */
	boolean canBeCalledFor( MethodSignature callSignature )
	{
		//System.err.println( "canBeCalledFor callee "+toString()+" caller "+callSignature.toString() );
		
		Iterator<PhantomType> callerTypeI = callSignature.args.iterator();
		for( PhantomType ourType : args )
		{
			if(!callerTypeI.hasNext())
				return false;
			
			PhantomType hisType = callerTypeI.next();
			
			if(!ourType.can_be_assigned_from(hisType))
				return false;
		}

		if(callerTypeI.hasNext())
			return false;
		
		return true;
	}

	public String getName() { return name; }
	
	@Override
	public String toString() { return getSignature(); }
	
}
