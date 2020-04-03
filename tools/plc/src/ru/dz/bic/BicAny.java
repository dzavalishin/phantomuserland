package ru.dz.bic;

import java.util.LinkedList;
import java.util.function.Consumer;

import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * 
 * AST tree or chain
 * 
 * @author dz
 *
 */

public abstract class BicAny 
{
	abstract void foreach(Consumer<? super BicAny> action);
	
	@Override
	abstract public String toString();
	
	public final void dump()
	{
		dump( 0 );
	}

	protected final void dump(int level) {
		for( int i = level; i > 0; i-- )
			System.out.print("  ");
		
		if( hasNode() ) System.out.println("+Node " + node.toString());
		else System.out.println("-Unconverted: "+ toString());
		
		dumpChildren(level+1);
	}

	protected abstract void dumpChildren(int i);

	/**
	 * Before convertExpressions()
	 * 
	 * Recursion is provided by implementation. 
	 * @throws PlcException 
	 */
	public void preprocess() throws PlcException {}
	
	
	public final void convertExpressions() 
	{
		foreach( el -> { 
			try {
				el.expressionToNode();
			} catch (PlcException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} 
			});		
	}

	
	protected Node node;
	
	protected abstract void expressionToNode() throws PlcException;
	
	public boolean hasNode() { return node != null; }
	
	public Node getNode() { return node; }

	private ParseState ps;
	public void setParseState(ParseState ps) { this.ps = ps; }
	protected ParseState getParseState() { return ps; }


	
	
}
